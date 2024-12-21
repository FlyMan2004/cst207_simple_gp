// Compile with: clang++-20 -std=gnu++23 -Og -fsized-deallocation -fuse-ld=lld -lbacktrace -lstdc++exp ${this_file}
#if __cplusplus <= 202002L
#error "This program requires at least C++23 or C++:latest"
#endif

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <exception>
#include <ios>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <ctime>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <chrono>
#include <limits>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <generator>
#include <ranges>
#include <source_location>
#include <stacktrace>
#ifdef linux
#include <sys/types.h>
#include <uuid/uuid.h>
#endif

// Forward declarations
class Book;
class Transaction;
class User;
class LibrarySystem;
class CSV;
class UUID;

// Error Handling
class LibraryException
  : public std::runtime_error
{
public:
  LibraryException() = delete;
  LibraryException(const LibraryException&) = default;
  LibraryException(LibraryException&&) noexcept = default;
  auto operator=(const LibraryException&) -> LibraryException& = default;
  auto operator=(LibraryException&&) noexcept -> LibraryException& = default;
  virtual ~LibraryException() = default;

  explicit LibraryException(
    std::string const& message = "",
    std::source_location const& source_location = std::source_location::current(),
    std::stacktrace const& stacktrace = std::stacktrace::current()
  ) noexcept
    : std::runtime_error(std::format(
        "source_location: {}({}:{}) `{}`: {}\n"
        "stacktrace: {}",
        source_location.file_name(), source_location.line(), source_location.column(), source_location.function_name(), message,
        stacktrace
      ))
  {}
  using std::runtime_error::what;
};

class [[deprecated("Use `UUID` instead")]] XorShift64
{
private:
  uint64_t m_seed;
  /* Magic numbers from [XORShift random number generators](https://www.javamex.com/tutorials/random_numbers/xorshift.shtml) */
  static constexpr uint64_t a = 21;
  static constexpr uint64_t b = 35;
  static constexpr uint64_t c = 4;
public:
  XorShift64() noexcept : m_seed(std::random_device{}()) {}
  XorShift64(uint64_t seed) noexcept : m_seed(seed) {}

  auto operator()() noexcept -> uint64_t
  {
    m_seed ^= m_seed << a;
    m_seed ^= m_seed >> b;
    m_seed ^= m_seed << c;
    return m_seed;
  }
};

[[deprecated("Use `UUID(gen_t{})` instead")]] static auto gen_random() noexcept -> uint64_t
{
  thread_local XorShift64 rng;
  return rng();
}

class UUID
{
  friend std::formatter<UUID>;
private:
  static constexpr size_t s_alignment = 16;
  static constexpr size_t s_size = sizeof(::uuid_t);
  alignas(s_alignment) std::array<uint8_t, s_size> m_uuid{};
  [[nodiscard]] static auto generate() noexcept -> std::array<uint8_t, s_size>
  {
    std::array<uint8_t, s_alignment> uuid{};
    ::uuid_generate_random(uuid.data());
    return uuid;
  }
public:
  struct gen_t {};
  UUID() noexcept = default;
  explicit UUID(gen_t) noexcept : m_uuid(generate()) {}
  UUID(UUID const&) noexcept = default;
  UUID(UUID&&) noexcept = default;
  auto operator=(UUID const&) noexcept -> UUID& = default;
  auto operator=(UUID&&) noexcept -> UUID& = default;
  ~UUID() = default;

  [[nodiscard]] friend auto operator<=>(UUID const&, UUID const&) noexcept = default;
  friend auto operator<<(std::ostream& os, UUID const& uuid) -> std::ostream&;
  friend auto operator>>(std::istream& is, UUID& uuid) -> std::istream&;
};
template<>
struct std::formatter<UUID>
{
  template<typename ParseContext>
  constexpr static auto parse(ParseContext& ctx) noexcept { return ctx.begin(); }
  template<typename FormatContext>
  static auto format(UUID const& uuid, FormatContext& ctx) noexcept -> decltype(auto)
  {
    static constexpr size_t buffer_size = 48;
    static constexpr size_t uuid_width = 36;
    static constexpr decltype(auto) fmt_str = "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}";
    std::array<char, buffer_size> buffer{};
    std::format_to_n(buffer.data(), buffer_size, fmt_str,
      uuid.m_uuid[0], uuid.m_uuid[1], uuid.m_uuid[2], uuid.m_uuid[3],
      uuid.m_uuid[4], uuid.m_uuid[5],
      uuid.m_uuid[6], uuid.m_uuid[7],
      uuid.m_uuid[8], uuid.m_uuid[9],
      uuid.m_uuid[10], uuid.m_uuid[11], uuid.m_uuid[12], uuid.m_uuid[13], uuid.m_uuid[14], uuid.m_uuid[15]
    );
    return std::ranges::copy_n(buffer.data(), uuid_width, ctx.out()).out;
  }
};
auto operator<<(std::ostream& os, UUID const& uuid) -> std::ostream&
{ return os << std::format("{}", uuid); }
auto operator>>(std::istream& is, UUID& uuid) -> std::istream&
{
  static constexpr size_t buffer_size = 48;
  static constexpr size_t uuid_width = 36;
  std::array<char, buffer_size> buffer{};
  is.read(buffer.data(), uuid_width);
  if (is.gcount() != uuid_width) {
    is.setstate(std::ios_base::failbit);
    return is;
  }
  if (buffer[8] != '-' || buffer[13] != '-' || buffer[18] != '-' || buffer[23] != '-') {
    is.setstate(std::ios_base::failbit);
    return is;
  }
  for (auto it = buffer.cbegin(); auto& value : uuid.m_uuid) {
    if (*it == '-') std::advance(it, 1);
    if (auto const [p, ec] = std::from_chars(it, std::next(it, 2), value, 16); ec != std::errc{}) {
      is.setstate(std::ios_base::failbit);
      return is;
    }
    std::advance(it, 2);
  }
  return is;
}

// Book Class
class Book
{
public:
  using id_type = uint32_t;
  using string_type = std::string;
private:
  static constexpr inline id_type s_invalid_id = 0;
  static constexpr inline id_type s_initial_id = 1000;
  static inline id_type s_available_id = s_initial_id;
  id_type m_id = s_invalid_id;
  bool m_available = true;
  string_type m_title{};
  string_type m_author{};
  string_type m_category{};

public:
  /* Rule of Five/Six */
  Book() = default;
  Book(const Book&) = default;
  Book(Book&&) noexcept = default;
  auto operator=(const Book&) -> Book& = default;
  auto operator=(Book&&) noexcept -> Book& = default;
  ~Book() = default;

  Book(
    id_type book_id,
    string_type book_title,
    string_type book_author,
    string_type book_category
  ) noexcept
    : m_id(book_id)
    , m_title(std::move(book_title))
    , m_author(std::move(book_author))
    , m_category(std::move(book_category))
  {}

  // Getters
  static auto get_available_id() noexcept -> id_type { return s_available_id++; }
  [[nodiscard]] auto get_id() const noexcept -> id_type { return m_id; }
  [[nodiscard]] auto get_title() const noexcept -> std::string_view { return m_title; }
  [[nodiscard]] auto get_author() const noexcept -> std::string_view { return m_author; }
  [[nodiscard]] auto get_category() const noexcept -> std::string_view { return m_category; }
  [[nodiscard]] auto is_available() const noexcept -> bool { return m_available; }

  // Setters
  void set_title(string_type newTitle) noexcept { m_title = std::move(newTitle); }
  void set_author(string_type newAuthor) noexcept { m_author = std::move(newAuthor); }
  void set_category(string_type newCategory) noexcept { m_category = std::move(newCategory); }
  void set_availability(bool status) noexcept { m_available = status; }
}; // class Book

// Transaction Class
class Transaction
{
public:
  using book_id_type = Book::id_type;
  using string_type = std::string;
private:
  string_type m_receipt_number{};
  string_type m_user_id{};
  string_type m_date{};
  book_id_type m_book_id{};
  bool has_returned{};

public:
  Transaction(
    string_type receipt,
    string_type user,
    book_id_type book,
    bool returnStatus,
    string_type date = ""
  ) : m_receipt_number(std::move(receipt))
    , m_user_id(std::move(user))
    , m_date(std::move(date))
    , m_book_id(book)
    , has_returned(returnStatus)
  {
    if (!this->m_date.empty()) return;
    auto const now = std::chrono::system_clock::now();
    auto const now_time = std::chrono::system_clock::to_time_t(now);
    m_date = ctime(&now_time);
  }

  [[nodiscard]] auto get_receipt_number() const -> std::string_view { return m_receipt_number; }
  [[nodiscard]] auto get_user_id() const -> std::string_view { return m_user_id; }
  [[nodiscard]] auto get_book_id() const -> book_id_type { return m_book_id; }
  [[nodiscard]] auto getDate() const -> std::string_view { return m_date; }
  [[nodiscard]] auto isReturnTransaction() const -> bool { return has_returned; }
};

// User Class
class User
{
public:
  using string_type = std::string;
private:
  string_type m_username{};
  string_type m_password{};
  bool m_is_admin{false};

public:
  User() = delete;
  User(const User&) = default;
  User(User&&) noexcept = default;
  auto operator=(const User&) -> User& = default;
  auto operator=(User&&) noexcept -> User& = default;
  User(
    string_type username,
    string_type password,
    bool is_admin
  ) noexcept
    : m_username(std::move(username))
    , m_password(std::move(password))
    , m_is_admin(is_admin)
  {}
  ~User() = default;

  [[nodiscard]] auto authenticate(std::string_view input_passwd) const noexcept -> bool
  { return m_password == input_passwd; }
  [[nodiscard]] auto is_admin() const noexcept -> bool { return m_is_admin; }
  [[nodiscard]] auto get_username() const noexcept -> std::string_view { return m_username; }
};

template <typename T>
concept parsable_from_string =
  std::convertible_to<std::string, T> || (
    std::is_default_constructible_v<T> && requires(T t) { std::stringstream{} >> t; }
  );

template <typename T>
concept parsable_to_string = requires(T const& t) { std::format("{}", t); };

class CSV
{
public:
  class record_view
    : private std::span<std::string const>
  {
  private:
    using base = std::span<std::string const>;
  protected:
    friend CSV;
    using base::base;
  public:
    using base::size;

    template <parsable_from_string T>
    [[nodiscard]] auto
    parse_as(size_t index) const -> T
    {
      if (index >= this->size()) {
        throw LibraryException("Index out of bounds");
      }
      if constexpr (std::convertible_to<std::string, T>) {
        return T{this->base::operator[](index)};
      } else {
        T result;
        std::stringstream ss(this->base::operator[](index));
        ss >> result;
        if (ss.fail()) {
          throw LibraryException("Error parsing field");
        }
        return result;
      }
    }
  };

  using string = std::string;

private:
  using record = std::vector<string>;
  static constexpr inline auto s_openmode = std::ios_base::binary | std::ios_base::in;

  std::filesystem::path m_filepath{};
  mutable std::ifstream m_file{};
  std::ifstream::pos_type m_record_start_pos{};
  record m_header{};
  mutable std::vector<record> m_records{};
  bool m_modified{};

public:
  CSV() = delete;
  CSV(const CSV&) = delete;
  CSV(CSV&&) noexcept = default;
  auto operator=(const CSV&) -> CSV& = delete;
  auto operator=(CSV&&) noexcept -> CSV& = default;
  ~CSV()
  {
    try {
      if (this->modified()) {
        this->save_changes();
      }
    } catch (std::exception const& e) {
      std::cerr << std::format("Error during destruction: {}\n", e.what());
      std::terminate();
    } catch (...) {
      std::cerr << "Unknown error during destruction\n";
      std::terminate();
    }
  }

  CSV(
    std::filesystem::path filepath,
    std::string_view expected_header = ""
  ) : m_filepath(std::move(filepath))
  {
    namespace fs = std::filesystem;
    if (!fs::exists(this->m_filepath)) {
      std::ofstream(this->m_filepath, std::ios_base::binary).close();
    }
    this->m_file.open(this->m_filepath, s_openmode);
    if (!this->m_file.is_open()) {
      throw LibraryException("Cannot open file for reading");
    }
    bool const file_empty = fs::file_size(m_filepath) == 0;
    bool const expected_empty = expected_header.empty();
    if (file_empty && expected_empty) {
      throw LibraryException("Empty file and empty header");
    }
    if (!file_empty) {
      std::string header_line;
      std::getline(this->m_file, header_line);
      this->m_file >> std::ws;
      this->m_record_start_pos = this->m_file.tellg();
      if (!expected_empty && header_line != expected_header) {
        throw LibraryException("Header mismatch");
      }
      this->m_header = parse_record_line(std::move(header_line));
    } else {
      this->m_header = parse_record_line(std::string{expected_header});
      this->m_modified = true;
      this->save_changes();
    }
  }

private:
  [[nodiscard]] static auto parse_record_line(string record_line) -> record
  {
    while (!record_line.empty() && !std::isgraph(record_line.back())) {
      record_line.pop_back();
    }
    record record{};
    std::istringstream ss(std::move(record_line));
    ss >> std::ws;
    for (string field_str; std::getline(ss, field_str, ','); field_str.clear()) {
      record.push_back(std::move(field_str));
    }
    if (!ss.eof() && ss.fail()) {
      throw LibraryException("Error parsing record line");
    }
    return record;
  }

  [[nodiscard]] auto parsed_record_count() const -> size_t { return this->m_records.size(); }

  [[nodiscard]] auto traverse_file() const -> std::generator<record>
  {
  #define assert(...) \
  if (!(__VA_ARGS__)) throw LibraryException("Assertion failed: " #__VA_ARGS__)
    if (this->all_parsed()) {
      co_return;
    }
    assert(this->m_record_start_pos != 0);
    assert(this->m_file.tellg() >= this->m_record_start_pos);
    for (std::string record_line; std::getline(this->m_file, record_line); record_line.clear()) {
      if (this->m_file.fail()) {
        throw LibraryException("Error reading file");
      }
      this->m_file >> std::ws;
      co_yield parse_record_line(std::move(record_line));
    }
    co_return;
  #undef assert
  }

  void checkout_to(std::filesystem::path const& new_filepath, std::ifstream::pos_type new_record_start_pos) noexcept
  {
    namespace fs = std::filesystem;
    try {
      this->m_file.close();
      fs::path old_filepath = this->m_filepath;
      old_filepath += ".old";
      fs::remove(old_filepath);
      fs::rename(this->m_filepath, old_filepath);
      fs::rename(new_filepath, this->m_filepath);
      this->m_file.open(this->m_filepath, s_openmode);
      if (!this->m_file.is_open()) {
        throw LibraryException("Cannot open file for reading/writing");
      }
      this->m_file.seekg(new_record_start_pos);
      this->m_record_start_pos = new_record_start_pos;
    } catch (std::exception const& e) {
      std::cerr << std::format("Error during checkout: {}\n", e.what());
      std::cerr << "Terminating program...\n";
      std::terminate();
    } catch (...) {
      std::cerr << "Unknown error during checkout\n";
      std::cerr << "Terminating program...\n";
      std::terminate();
    }
  }

public:
  [[nodiscard]] auto get_header() const -> record_view { return this->m_header; }

  [[nodiscard]] auto field_count() const -> size_t { return this->m_header.size(); }

  [[nodiscard]] auto record_count() const -> size_t
  {
    if (!this->all_parsed()) {
      throw LibraryException("Records not fully parsed");
    }
    return this->m_records.size();
  }

  [[nodiscard]] auto all_parsed() const -> bool { return this->m_file.eof(); }

  [[nodiscard]] auto modified() const -> bool { return this->m_modified; }

  [[nodiscard]] auto traverse_records(size_t from = 0) const -> std::generator<record_view>
  {
    for (auto const& existing_record : this->m_records | std::views::drop(from)) {
      co_yield existing_record;
    }
    for (size_t index = this->parsed_record_count(); auto new_record : this->traverse_file()) {
      this->m_records.push_back(std::move(new_record));
      if (index < from) {
        continue;
      }
      co_yield this->m_records.back();
    }
    co_return;
  }

  void parse_all_records()
  {
    for ([[maybe_unused]] auto const& _ : this->traverse_records(this->parsed_record_count())) {}
  }

  /* Clear all records. A checkout point */
  void clear_records()
  {
    this->m_modified = true;
    this->m_records.clear();
    this->save_changes();
  }

  template <parsable_to_string... Ts>
  void append_record(Ts&&... fields)
  {
    if (sizeof...(Ts) != this->field_count()) {
      throw LibraryException("Field count mismatch");
    }
    if (!this->modified() || !this->all_parsed()) this->parse_all_records();
    this->m_modified = true;
    this->m_records.push_back({std::format("{}", std::forward<Ts>(fields))...});
  }

  template <parsable_to_string T>
  void modify_at(size_t record_index, size_t field_index, T&& new_value)
  {
    if (this->all_parsed() && record_index >= this->record_count()) {
      throw LibraryException("Record index out of bounds: `record_index` >= `record_count`");
    }
    if (!this->all_parsed() && record_index >= this->parsed_record_count()) {
      throw LibraryException("Record index out of bounds: current record has not been parsed");
    }
    if (field_index >= this->field_count()) {
      throw LibraryException("Field index out of bounds");
    }
    this->m_modified = true;
    this->m_records[record_index][field_index] = std::format("{}", std::forward<T>(new_value));
  }

  template <parsable_to_string T>
  void modify_at(size_t record_index, std::string_view field_name, T&& new_value)
  {
    auto const field_pos = std::ranges::find(this->m_header, field_name);
    if (field_pos == this->m_header.end()) {
      throw LibraryException("Field name not found");
    }
    auto const field_index = std::distance(this->m_header.begin(), field_pos);
    this->modify_at(record_index, field_index, std::forward<T>(new_value));
  }

  template <parsable_to_string... Ts>
  void modify_at(size_t record_index, Ts&&... new_values)
  {
    if (sizeof...(Ts) != this->field_count()) {
      throw LibraryException("Field count mismatch");
    }
    if (this->all_parsed() && record_index >= this->record_count()) {
      throw LibraryException("Record index out of bounds: `record_index` >= `record_count`");
    }
    if (!this->all_parsed() && record_index >= this->parsed_record_count()) {
      throw LibraryException("Record index out of bounds: current record has not been parsed");
    }
    this->m_modified = true;
    auto& record = this->m_records[record_index];
    [&]<std::size_t... Is>(std::index_sequence<Is...>)
    {
      ((record[Is] = std::format("{}", std::forward<Ts>(new_values))), ...);
    }(std::index_sequence_for<Ts...>{});
  }

  void discard_changes() noexcept
  {
    try {
      this->m_records.clear();
      this->m_file.clear();
      this->m_file.seekg(this->m_record_start_pos);
      this->m_modified = false;
    } catch (std::exception const& e) {
      std::cerr << std::format("Error during discarding changes: {}\n", e.what());
      std::cerr << "Terminating program...\n";
      std::terminate();
    } catch (...) {
      std::cerr << "Unknown error during discarding changes\n";
      std::cerr << "Terminating program...\n";
      std::terminate();
    }
  }

  /* Write changes to a new file. A checkout point */
  void save_changes()
  {
    namespace fs = std::filesystem;
    if (!this->modified()) return;
    /* Create new file to save changes */
    fs::path new_filepath = this->m_filepath;
    new_filepath += ".new";
    std::ofstream new_file(new_filepath, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    if (!new_file.is_open()) {
      throw LibraryException("Cannot open file for saving changes");
    }
    /* Write changes to new file */
    /* Write the header line */
    for (bool is_first = true; auto const& field : this->m_header) {
      auto const str = std::format("{}{}", is_first ? "" : ",", field);
      new_file << str;
      is_first = false;
    }
    new_file << '\n';
    /* Get the position of the beginning of records */
    auto const new_record_start_pos = new_file.tellp();
    /* Write the records */
    for (auto const& record : this->m_records) {
      for (bool is_first = true; auto const& field : record) {
        auto const str = std::format("{}{}", is_first ? "" : ",", field);
        new_file << str;
        is_first = false;
      }
      new_file << '\n';
    }
    /* Finished. Clear the flag. */
    this->m_modified = false;
    /* Replace old file with new file */
    this->checkout_to(new_filepath, new_record_start_pos);
  }
};

class getline_t
{
  std::reference_wrapper<std::string> str;

public:
  explicit getline_t(std::string& s) noexcept : str(std::ref(s)) {}
  friend auto operator>>(std::istream& is, getline_t const& str_wrapper) -> std::istream&
  {
    is >> std::ws;
    decltype(auto) ret = std::getline(is, str_wrapper.str.get());
    return ret;
  }
};

class input_t
{
  std::reference_wrapper<std::istream> is;

public:
  explicit input_t(std::istream& is) noexcept : is(std::ref(is)) {}
  template <typename... Ts>
  auto operator()(Ts&&... args) -> decltype(auto)
  {
    decltype(auto) input = (is.get() >> ... >> std::forward<Ts>(args));
    if (input.eof()) {
      throw std::ios_base::failure("EOF reached");
    }
    if (!input.good()) {
      throw LibraryException("Invalid input");
    }
    return (*this);
  }

  auto prompt(std::string_view prompt) -> decltype(auto)
  {
    std::cout << prompt;
    return (*this);
  }

  template <typename... Ts>
  auto get(Ts&&... args) -> decltype(auto)
  {
    return (*this)(std::forward<Ts>(args)...);
  }

  auto reset() -> decltype(auto)
  {
    is.get().clear();
    is.get().ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return (*this);
  }
};

// Library System Class
class LibrarySystem
{
public:
  // Add file path constants
  static constexpr inline std::string_view S_BOOKS_FILE = "books.csv";
  static constexpr inline std::string_view S_CSV_HEADER_BOOKS = "ID,Title,Author,Category,Available";
  static constexpr inline std::string_view S_TRANSACTIONS_FILE = "transactions.csv";
  static constexpr inline std::string_view S_CSV_HEADER_TRANSACTIONS = "Receipt,UserID,BookID,Date,Type";

private:
  CSV m_books_file{S_BOOKS_FILE, S_CSV_HEADER_BOOKS};
  CSV m_transactions_file{S_TRANSACTIONS_FILE, S_CSV_HEADER_TRANSACTIONS};
  std::vector<Book> m_books{};
  std::vector<Transaction> m_transactions{};
  std::vector<User> m_users{};
  User const* m_curr_user{};

public:
  LibrarySystem()
  {
    // Add default users
    m_users.emplace_back("admin", "123456", true);
    m_users.emplace_back("user", "123456", false);

    // Load book data
    load_books();
    load_transactions();
  }
  LibrarySystem(const LibrarySystem&) = delete;
  LibrarySystem(LibrarySystem&&) noexcept = default;
  auto operator=(const LibrarySystem&) -> LibrarySystem& = delete;
  auto operator=(LibrarySystem&&) noexcept -> LibrarySystem& = default;

  ~LibrarySystem()
  {
    logout();
    // Save book data
    save_books();
    save_transactions();
  }

private:
  // Sorting Algorithms
  static void merge_sort(std::vector<Book>& book_list, size_t left, size_t right) noexcept
  {
    if (left >= right) return;

    size_t const mid = left + (right - left) / 2;
    merge_sort(book_list, left, mid);
    merge_sort(book_list, mid + 1, right);

    std::vector<Book> temp(right - left + 1);
    size_t i = left, j = mid + 1, k = 0;

    while (i <= mid && j <= right) {
      if (book_list[i].get_id() <= book_list[j].get_id())
        temp[k++] = book_list[i++];
      else
        temp[k++] = book_list[j++];
    }

    while (i <= mid) temp[k++] = book_list[i++];
    while (j <= right) temp[k++] = book_list[j++];

    for (i = 0; i < k; i++)
      book_list[left + i] = temp[i];
  }

  static void bubble_sort(std::vector<Book>& book_list) noexcept
  {
    size_t const n = book_list.size();
    for (size_t i = 0; i < n - 1; i++) {
      for (size_t j = 0; j < n - i - 1; j++) {
        if (book_list[j].get_title() > book_list[j + 1].get_title())
          std::swap(book_list[j], book_list[j + 1]);
      }
    }
  }

  // Search Algorithms
  static auto binary_search(std::vector<Book> const& book_list, Book::id_type target_id) noexcept -> size_t
  {
    if (!std::ranges::is_sorted(book_list, {}, &Book::get_id)) {
      std::cerr << LibraryException("List is not sorted").what() << '\n';
      std::terminate();
    }
    size_t left = 0;
    size_t right = book_list.size() - 1;

    while (left <= right) {
      size_t const mid = left + (right - left) / 2;
      if (book_list[mid].get_id() == target_id)
        return mid;
      if (book_list[mid].get_id() < target_id)
        left = mid + 1;
      else
        right = mid - 1;
    }

    return -1;
  }

  // Add save and load functions
  void save_books()
  {
    merge_sort(this->m_books, 0, m_books.size() - 1);
    m_books_file.clear_records();
    for (const auto& book : m_books) {
      m_books_file.append_record(
        book.get_id(),
        book.get_title(),
        book.get_author(),
        book.get_category(),
        book.is_available() ? 1 : 0
      );
    }
  }

  void load_books()
  {
    for (auto record : m_books_file.traverse_records()) {
      m_books.emplace_back(
        record.parse_as<Book::id_type>(0),
        record.parse_as<Book::string_type>(1),
        record.parse_as<Book::string_type>(2),
        record.parse_as<Book::string_type>(3)
      ).set_availability(record.parse_as<bool>(4));
    }
    m_books_file.discard_changes();
  }

  void save_transactions()
  {
    m_transactions_file.clear_records();
    for (auto const& trans : m_transactions) {
      m_transactions_file.append_record(
        trans.get_receipt_number(),
        trans.get_user_id(),
        trans.get_book_id(),
        trans.getDate(),
        trans.isReturnTransaction() ? "Return" : "Borrow"
      );
    }
  }

  void load_transactions()
  {
    for (auto record : m_transactions_file.traverse_records()) {
      m_transactions.emplace_back(
        record.parse_as<Transaction::string_type>(0),
        record.parse_as<Transaction::string_type>(1),
        record.parse_as<Transaction::book_id_type>(2),
        record.parse_as<std::string>(4) == "Return",
        record.parse_as<Transaction::string_type>(3)
      );
    }
    m_transactions_file.discard_changes();
  }

public:
  auto login(std::string_view username, std::string_view password) noexcept -> bool
  {
    for (auto const& user : m_users) {
      if (user.get_username() == username && user.authenticate(password)) {
        m_curr_user = &user;
        return true;
      }
    }
    return false;
  }

  void logout() noexcept
  { m_curr_user = nullptr; }

  // Administrator Functions
  auto add_book(Book book) -> bool
  {
    if (m_curr_user == nullptr || !m_curr_user->is_admin()) {
      throw LibraryException("Unauthorized access");
    }
    m_books.push_back(std::move(book));
    save_books(); // Save changes immediately
    return true;
  }

  auto remove_book(Book::id_type m_id) -> bool
  {
    if (!m_curr_user || !m_curr_user->is_admin()) {
      throw LibraryException("Unauthorized access");
    }
    size_t const index = binary_search(m_books, m_id);
    using difference_type = std::vector<Book>::difference_type;
    if (index != -1zu) {
      m_books.erase(m_books.begin() + static_cast<difference_type>(index));
      save_books();
      return true;
    }
    return false;
  }

  void display_all_books() const noexcept
  {
    if (m_books.empty()) {
      std::cout << "No books in the library.\n";
      return;
    }

    // std::cout << "\n\t\t\t\t\t\t=== All Books ===\n";
    std::cout << std::format(
      "\n"
      "{:>50}=== All Books ===\n",
      ""
    );
    static constexpr auto fmt_string = "{:>10} {:>40} {:>30} {:>20} {:>15}\n";
    std::cout << std::format(
      fmt_string,
      "ID", "Title", "Author", "Category", "Status"
    );
    // cout << string(90, '-') << "\n";
    std::cout << std::format("{:->120}\n", "");

    for (const auto& book : m_books) {
      std::cout << std::format(
        fmt_string,
        book.get_id(),
        book.get_title(),
        book.get_author(),
        book.get_category(),
        book.is_available() ? "Available" : "Borrowed"
      );
    }
  }

  void sort_and_display_books(std::string_view sort_by) noexcept
  {
    if (m_books.empty()) {
      std::cout << "No books in the library.\n";
      return;
    }
    using books_type = std::vector<Book>;
    books_type books_copy;
    auto books = std::ref(m_books);
    if (sort_by == "m_id") {
      size_t const left = 0;
      size_t const right = books.get().size() - 1;
      merge_sort(books, left, right);
    } else if (sort_by == "m_title") {
      books_copy = m_books;
      books = std::ref(books_copy);
      bubble_sort(books);
    }

    std::cout << std::format(
      "\n"
      "\t\t\t\t=== Sorted Books ({}) ===\n",
      sort_by
    );
    std::cout << std::format(
      "{:>10} {:>20} {:>20} {:>15} {:>15}\n",
      "ID", "Title", "Author", "Category", "Status"
    );
    std::cout << std::format("{:->90}\n", "");

    for (const auto& book : books.get()) {
      std::cout << std::format(
        "{:>10} {:>20} {:>20} {:>15} {:>15}\n",
        book.get_id(),
        book.get_title(),
        book.get_author(),
        book.get_category(),
        book.is_available() ? "Available" : "Borrowed"
      );
    }
  }

  void view_all_transactions() const noexcept
  {
    if (m_transactions.empty()) {
      std::cout << "No transaction.\n";
      return;
    }

    std::cout << std::format(
      "\n"
      "\t\t\t\t=== Transaction Records ===\n"
    );
    std::cout << std::format(
      "{:>15} {:>15} {:>15} {:>25} {:>10}\n",
      "Receipt", "User name", "Book ID", "Date", "Type"
    );
    std::cout << std::format("{:->80}\n", "");

    for (const auto& trans : m_transactions) {
      std::cout << std::format(
        "{:>15} {:>15} {:>15} {:>25} {:>10}\n",
        trans.get_receipt_number(),
        trans.get_user_id(),
        trans.get_book_id(),
        trans.getDate(),
        trans.isReturnTransaction() ? "Return" : "Borrow"
      );
    }
  }

  auto modify_book(Book::id_type book_id, Book new_book_info) -> bool
  {
    if (!m_curr_user || !m_curr_user->is_admin()) {
      throw LibraryException("Unauthorized access");
    }
    size_t const index = binary_search(m_books, book_id);
    if (index != -1zu) {

      m_books[index] = std::move(new_book_info);
      save_books();
      return true;
    }
    return false;
  }

  // User Functions
  auto search_book_by_id(Book::id_type book_id) noexcept -> Book*
  {
    size_t const index = binary_search(m_books, book_id);
    if (index != -1zu) {
      return &m_books[index];
    }
    return nullptr;
  }
  auto borrow_book(Book::id_type book_id, User::string_type user_id) -> bool
  {
    size_t const index = binary_search(m_books, book_id);
    if (index != -1zu && m_books[index].is_available()) {
      m_books[index].set_availability(false);
      std::string receipt = std::format("{}", gen_random());
      m_transactions.emplace_back(receipt, std::move(user_id), book_id, false);
      save_books();
      save_transactions();
      return true;
    }
    return false;
  }

  auto return_book(Book::id_type book_id, User::string_type user_id) -> bool
  {
    size_t const index = binary_search(m_books, book_id);
    if (index != -1zu && !m_books[index].is_available()) {
      m_books[index].set_availability(true);
      std::string receipt = std::format("{}", gen_random());
      m_transactions.emplace_back(receipt, std::move(user_id), book_id, true);
      save_books();
      save_transactions();
      return true;
    }
    return false;
  }

  void view_transactions_of(std::string_view user_id) const noexcept
  {
    if (this->m_transactions.empty()) {
      std::cout << "No transactions found.\n";
      return;
    }
    for (const auto& trans : m_transactions) {
      if (trans.get_user_id() == user_id) {
        std::cout << std::format(
          "Receipt: {:>15} Book: {:>15} Date: {:>25} Type: {:>10}\n",
          trans.get_receipt_number(),
          trans.get_book_id(),
          trans.getDate(),
          trans.isReturnTransaction() ? "Return" : "Borrow"
        );
      }
    }
  }

};

// Helper Functions

void display_main_menu() noexcept
{
  std::cout << std::format(
    "\n"
    "=== Library Management System ===\n"
    "1. Administrator Login\n"
    "2. User Login\n"
    "0. Exit\n"
    "Please select an option: "
  );
}

void display_admin_menu() noexcept
{
  std::cout << std::format(
    "\n"
    "=== Administrator Control Panel ===\n"
    "1. Add New Book\n"
    "2. Search Book\n"
    "3. Edit Book Details\n"
    "4. Delete Book\n"
    "5. View All Books\n"
    "6. Sort Books by ID (Merge Sort)\n"
    "7. Sort Books by Title (Bubble Sort)\n"
    "8. View Transaction Records\n"
    "9. Search Transaction Records\n"
    "0. Return to Main Menu\n"
    "Please select an option: "
  );
}

void display_user_menu() noexcept
{
  std::cout << std::format(
    "\n"
    "=== User Control Panel ===\n"
    "1. Search Book\n"
    "2. Borrow Book\n"
    "3. View Borrowing History\n"
    "4. Return Book\n"
    "0. Return to Main Menu\n"
    "Please select an option: "
  );
}

void handle_admin_menu(LibrarySystem& system)
{
  enum class AdminChoice : uint8_t
  {
    EXIT                    = 0,
    ADD_BOOK                = 1,
    SEARCH_BOOK             = 2,
    MODIFY_BOOK             = 3,
    REMOVE_BOOK             = 4,
    DISPLAY_ALL_BOOKS       = 5,
    SORT_BY_ID              = 6,
    SORT_BY_TITLE           = 7,
    VIEW_ALL_TRANSACTIONS   = 8,
    VIEW_USER_TRANSACTIONS  = 9,
  };
  AdminChoice choice{};
  input_t input(std::cin);

  auto const get_choice = [&input, &choice]
  {
    uint16_t choice_base = 0; /* Cannot use `uint8_t` as it's `unsigned char`... */
    for (bool no_error = false; !no_error; ) {
      try { input(choice_base); no_error = true; }
      catch (LibraryException const& ) {
        input.reset().prompt("Invalid input. Please enter a number: ");
      }
    }
    choice = static_cast<AdminChoice>(choice_base);
  };
  auto const add_book = [&system, &input]
  {
    Book::string_type title, author, category;
    input.prompt("Enter Title: ").get(getline_t{title});
    input.prompt("Enter Author: ").get(getline_t{author});
    input.prompt("Enter Category: ").get(getline_t{category});

    Book newBook(Book::get_available_id(), title, author, category);
    if (system.add_book(std::move(newBook))) {
      std::cout << "Book added successfully!\n";
    }
  };
  auto const search_book = [&system, &input]
  {
    Book::id_type book_id{};
    input.prompt("Enter book ID to search: ").get(book_id);
    Book const * const book = system.search_book_by_id(book_id);
    if (book == nullptr) {
      std::cout << "Book not found.\n";
      return;
    }
    std::cout << std::format(
      "Book found:\n"
      "ID: {}\n"
      "Title: {}\n"
      "Author: {}\n"
      "Category: {}\n"
      "Status: {}\n",
      book->get_id(),
      book->get_title(),
      book->get_author(),
      book->get_category(),
      book->is_available() ? "Available" : "Borrowed"
    );
  };
  auto const modify_book = [&system, &input]
  {
    Book::id_type book_id{};
    Book::string_type title, author, category;
    input.prompt("Enter Book ID to edit: ").get(book_id);
    input.prompt("Enter new Title: ").get(getline_t{title});
    input.prompt("Enter new Author: ").get(getline_t{author});
    input.prompt("Enter new Category: ").get(getline_t{category});

    Book newDetails(book_id, std::move(title), std::move(author), std::move(category));
    if (system.modify_book(book_id, std::move(newDetails))) {
      std::cout << "Book modified successfully!\n";
    } else {
      std::cout << "Book not found.\n";
    }
  };
  auto const remove_book = [&system, &input]
  {
    Book::id_type book_id{};
    input.prompt("Enter Book ID to delete: ").get(book_id);
    if (system.remove_book(book_id)) {
      std::cout << "Book removed successfully!\n";
    } else {
      std::cout << "Book not found.\n";
    }
  };
  auto const view_user_transactions = [&system, &input]
  {
    User::string_type user_id;
    input.prompt("Enter User ID to search: ").get(getline_t{user_id});
    system.view_transactions_of(user_id);
  };
  while (true) {
    display_admin_menu();
    get_choice();

    try {
      switch (choice) {
        using enum AdminChoice;
      case ADD_BOOK:
        add_book();
        break;
      case SEARCH_BOOK:
        search_book();
        break;
      case MODIFY_BOOK:
        modify_book();
        break;
      case REMOVE_BOOK:
        remove_book();
        break;
      case DISPLAY_ALL_BOOKS:
        system.display_all_books();
        break;
      case SORT_BY_ID:
        system.sort_and_display_books("m_id");
        break;
      case SORT_BY_TITLE:
        system.sort_and_display_books("m_title");
        break;
      case VIEW_ALL_TRANSACTIONS:
        system.view_all_transactions();
        break;
      case VIEW_USER_TRANSACTIONS:
        view_user_transactions();
        break;
      case EXIT:
        return;
      default:
        std::cout << "Invalid choice. Please try again.\n";
      }
    } catch (const LibraryException& e) {
      std::cout << std::format("Error: {}\n", e.what());
    }
  }
}

void handle_user_menu(LibrarySystem& system)
{
  enum class UserChoice : uint8_t
  {
    EXIT                  = 0,
    SEARCH_BOOK           = 1,
    BORROW_BOOK           = 2,
    VIEW_HISTORY          = 3,
    RETURN_BOOK           = 4,
  };
  UserChoice choice{};
  input_t input(std::cin);
  auto const get_choice = [&input, &choice]
  {
    uint16_t choice_base = 0; /* Cannot use `uint8_t` as it's `unsigned char`... */
    for (bool no_error = false; !no_error; ) {
      try { input(choice_base); no_error = true; }
      catch (LibraryException const& ) {
        input.reset().prompt("Invalid input. Please enter a number: ");
      }
    }
    choice = static_cast<UserChoice>(choice_base);
  };
  auto const search_book = [&system, &input]
  {
    Book::id_type book_id{};
    input.prompt("Enter book ID to search: ").get(book_id);
    Book const * const book = system.search_book_by_id(book_id);
    if (book == nullptr) {
      std::cout << "Book not found.\n";
      return;
    }
    std::cout << std::format(
      "Book found:\n"
      "ID: {}\n"
      "Title: {}\n"
      "Author: {}\n"
      "Category: {}\n"
      "Status: {}\n",
      book->get_id(),
      book->get_title(),
      book->get_author(),
      book->get_category(),
      book->is_available() ? "Available" : "Borrowed"
    );
  };
  auto const borrow_book = [&system, &input]
  {
    Book::id_type book_id{};
    User::string_type user_id;
    input.prompt("Enter Book ID: ").get(book_id);
    input.prompt("Enter your User name: ").get(getline_t{user_id});

    if (system.borrow_book(book_id, std::move(user_id))) {
      std::cout << "Book borrowed successfully!\n";
    } else {
      std::cout << "Failed to borrow book. Book may not exist or is already borrowed.\n";
    }
  };
  auto const view_history = [&system, &input]
  {
    User::string_type user_id;
    input.prompt("Enter your User name: ").get(getline_t{user_id});
    system.view_transactions_of(user_id);
  };
  auto const return_book = [&system, &input]
  {
    Book::id_type book_id{};
    User::string_type user_id;
    input.prompt("Enter Book ID: ").get(book_id);
    input.prompt("Enter your User name: ").get(getline_t{user_id});

    if (system.return_book(book_id, std::move(user_id))) {
      std::cout << "Book returned successfully!\n";
    } else {
      std::cout << "Failed to return book. Book may not exist or is not borrowed.\n";
    }
  };
  while (true) {
    display_user_menu();
    get_choice();
    try {
      switch (choice) {
        using enum UserChoice;
      case SEARCH_BOOK:
        search_book();
        break;
      case BORROW_BOOK:
        borrow_book();
        break;
      case VIEW_HISTORY:
        view_history();
        break;
      case RETURN_BOOK:
        return_book();
        break;
      case EXIT:
        return;
      default:
        std::cout << "Invalid choice. Please try again.\n";
      }
    } catch (const LibraryException& e) {
      input.reset();
      std::cout << std::format("Error: {}\n", e.what());
    }
  }
}

[[maybe_unused]] static auto project_main() -> int __asm__("project_main");
static auto project_main() -> int
try {
  LibrarySystem system;
  User::string_type username, password;
  uint16_t choice = 0;
  input_t input{std::cin};
  static auto const is_valid_choice = [](int choice) static noexcept -> bool
  {
    switch (choice) {
    case 0: [[fallthrough]];
    case 1: [[fallthrough]];
    case 2:
      return true;
    default:
      return false;
    }
  };
  auto const get_choice = [&input, &choice]
  {
    for (bool valid = false; !valid; ) {
      try{ input(choice); valid = true; }
      catch (LibraryException const& e) {
        input.reset().prompt("Invalid input. Please enter a number: ");
      }
    }
  };

  while (true) {
    display_main_menu();
    try { get_choice(); }
    catch (std::ios_base::failure const&) { break; }
    try {
      if (!is_valid_choice(choice)) {
        std::cout << "Invalid choice. Please try again.\n";
        continue;
      }
      if (choice == 0) {
        break;
      }
      if (choice == 1 || choice == 2) {
        input.prompt("Username: ").get(getline_t{username});
        input.prompt("Password: ").get(getline_t{password});
      }
      bool const logged_in = system.login(username, password);
      if (!logged_in) {
        std::cout << "Invalid username or password\n";
        continue;
      }
      if (choice == 1) {
        handle_admin_menu(system);
      } else /* if (choice == 2) */ {
        handle_user_menu(system);
      }
      system.logout();
    } catch (LibraryException const& e) {
      std::cout << std::format("Error: {}\n", e.what());
    }
  }

  std::cout << "Thank you for using the Library Management System!\n";
  return 0;
} catch (std::ios_base::failure const& e) {
  std::cerr << "Unrecoverable IO Error: " << e.what() << '\n';
  return 1;
} catch (std::exception const& e) {
  std::cerr << "Error: " << e.what() << '\n';
  return 1;
} catch (...) {
  std::cerr << "Unknown error\n";
  return 1;
}

[[maybe_unused]] static auto test_uuid() -> int __asm__("test_uuid");
static auto test_uuid() -> int
{
  std::cout << "=== Testing UUID Generation ===\n";
  std::vector<UUID> uuids;
  for (auto const _ : std::views::iota(0, 10)) {
    uuids.emplace_back(UUID::gen_t{});
  }
  std::ranges::sort(uuids);
  for (auto const& uuid : uuids) {
    std::cout << std::format("UUID: {}\n", uuid);
  }
  std::cout << "=== Testing UUID Input ===\n";
  UUID uuid;
  std::cin >> uuid;
  std::cout << "UUID: " << uuid << '\n';
  return 0;
}

#ifdef TEST_UUID
#define TEST TEST_UUID
__attribute__((alias("test_uuid")))
auto main() -> int;
#endif

#ifndef TEST
__attribute__((alias("project_main")))
auto main() -> int;
#endif

module;
/* C++ language support */
#include <concepts>
#include <utility>
#include <generator>
/* IO and filesystem */
#include <filesystem>
#include <fstream>
#include <iostream>
/* C++ containers */
#include <vector>
#include <span>
#include <string>
#include <string_view>
export module CSV;
import base;

template <typename T>
concept parsable_from_string =
  std::convertible_to<std::string, T> || (
    std::is_default_constructible_v<T> && requires(T t) { std::stringstream{} >> t; }
  );

template <typename T>
concept parsable_to_string = requires(T const& t) { std::format("{}", t); };

export class CSV
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
}; // class CSV

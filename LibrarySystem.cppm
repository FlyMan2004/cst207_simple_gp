module;
#include <string>
#include <string_view>
#include <expected>
#include <chrono>
#include <iostream>
#include <utility>
export module LibrarySystem;
export import base;
export import Algorithms;
export import UUID;
import CSV;

// Book Class
export class Book
{
public:
  using id_type = UUID_ns::UUID;
  using string_type = std::string;
  static constexpr decltype(auto) fmt_string = "{} {:>40} {:>30} {:>20} {:>15}";
private:
  id_type m_id{};
  string_type m_title{};
  string_type m_author{};
  string_type m_category{};
  bool m_available = true;

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
  static auto get_available_id() noexcept -> id_type { return id_type{id_type::gen_t{}}; }
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

// User Class
export class User
{
public:
  using string_type = std::string;
  using string_view = std::string_view;
  using username_type = string_type;
  using password_type = string_type;
private:
  username_type m_username{};
  password_type m_password{};
  bool m_is_admin{false};

public:
  User() = delete;
  User(const User&) = default;
  User(User&&) noexcept = default;
  auto operator=(const User&) -> User& = default;
  auto operator=(User&&) noexcept -> User& = default;
  User(
    username_type username,
    password_type password,
    bool is_admin
  ) noexcept
    : m_username(std::move(username))
    , m_password(std::move(password))
    , m_is_admin(is_admin)
  {}
  ~User() = default;

  [[nodiscard]] auto authenticate(string_view input_passwd, bool priviledged = false) const noexcept -> bool
  { return this->is_admin() == priviledged && m_password == input_passwd; }
  [[nodiscard]] auto is_admin() const noexcept -> bool { return m_is_admin; }
  [[nodiscard]] auto get_username() const noexcept -> string_view { return m_username; }
}; // class User

// Transaction Class
export class Transaction
{
private:
  using clock_type = std::chrono::system_clock;
public:
  using string_type = std::string;
  using string_view = std::string_view;
  using receipt_id_type = UUID_ns::UUID;
  using username_type = User::string_type;
  using date_type = string_type;
  using book_id_type = Book::id_type;
  static constexpr decltype(auto) fmt_string = "{} {:>16} {} {:>40} {:>10}";
private:
  receipt_id_type m_receipt_number{};
  username_type m_username{};
  date_type m_date{};
  book_id_type m_book_id{};
  bool m_has_returned{};

public:
  Transaction(
    receipt_id_type receipt,
    username_type username,
    book_id_type book,
    bool return_status,
    date_type date = std::format("{}", clock_type::now())
  ) : m_receipt_number(receipt)
    , m_username(std::move(username))
    , m_date(std::move(date))
    , m_book_id(book)
    , m_has_returned(return_status)
  {}

  [[nodiscard]] auto get_receipt_number() const -> receipt_id_type { return m_receipt_number; }
  [[nodiscard]] auto get_user_id() const -> User::string_view { return m_username; }
  [[nodiscard]] auto get_book_id() const -> book_id_type { return m_book_id; }
  [[nodiscard]] auto get_date() const -> string_view { return m_date; }
  [[nodiscard]] auto has_returned() const -> bool { return m_has_returned; }
}; // class Transaction

// Library System Class
export class LibrarySystem
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
    m_users.emplace_back("student", "123456", false);
    m_users.emplace_back("staff", "123456", false);
    m_users.emplace_back("faculty", "123456", false);

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
  // Add save and load functions
  void save_books()
  {
    Algorithms::merge_sort(this->m_books, 0, m_books.size());
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
        trans.get_date(),
        trans.has_returned() ? "Return" : "Borrow"
      );
    }
  }

  void load_transactions()
  {
    for (auto record : m_transactions_file.traverse_records()) {
      m_transactions.emplace_back(
        record.parse_as<Transaction::receipt_id_type>(0),
        record.parse_as<Transaction::username_type>(1),
        record.parse_as<Transaction::book_id_type>(2),
        record.parse_as<std::string>(4) == "Return",
        record.parse_as<Transaction::date_type>(3)
      );
    }
    m_transactions_file.discard_changes();
  }

public:
  auto login(std::string_view username, std::string_view password, bool priviledged = false) noexcept -> bool
  {
    for (auto const& user : m_users) {
      if (user.get_username() == username && user.authenticate(password, priviledged)) {
        m_curr_user = &user;
        return true;
      }
    }
    return false;
  }

  auto current_user() const noexcept -> User const*
  { return m_curr_user; }

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
    size_t const index = Algorithms::binary_search(std::span{std::as_const(m_books)}, m_id, {}, &Book::get_id);
    using difference_type = std::vector<Book>::difference_type;
    auto const book_it = m_books.begin() + static_cast<difference_type>(index);
    if (book_it != m_books.end() && book_it->is_available()) {
      m_books.erase(book_it);
      save_books();
      return true;
    }
    return false;
  }

  void view_all_books() const noexcept
  {
    if (m_books.empty()) {
      std::cout << "No books in the library.\n";
      return;
    }

    std::cout << std::format(
      "\n"
      "{:>70}=== All Books ===\n",
      ""
    );
    std::string const ID_string = std::format("{:>30}", "ID");
    std::cout << std::format(
      Book::fmt_string,
      ID_string, "Title", "Author", "Category", "Status"
    ) << '\n';
    std::cout << std::format("{:->145}\n", "");

    for (const auto& book : m_books) {
      std::cout << std::format(
        Book::fmt_string,
        book.get_id(),
        book.get_title(),
        book.get_author(),
        book.get_category(),
        book.is_available() ? "Available" : "Borrowed"
      ) << '\n';
    }
  }

  void sort_and_view_books(std::string_view sort_by) noexcept
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
      size_t const right = books.get().size();
      Algorithms::merge_sort(books.get(), left, right);
    } else if (sort_by == "m_title") {
      books_copy = m_books;
      books = std::ref(books_copy);
      Algorithms::bubble_sort(books.get());
    }

    std::cout << std::format(
    "\n"
      "{:>60}=== Sorted Books ({}) ===\n",
      "", sort_by
    );

    std::string const ID_string = std::format("{:>30}", "ID");
    std::cout << std::format(
      Book::fmt_string,
      ID_string, "Title", "Author", "Category", "Status"
    ) << '\n';
    std::cout << std::format("{:->145}\n", "");

    for (const auto& book : books.get()) {
      std::cout << std::format(
        Book::fmt_string,
        book.get_id(),
        book.get_title(),
        book.get_author(),
        book.get_category(),
        book.is_available() ? "Available" : "Borrowed"
      ) << '\n';
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
      "{:>50}=== Transaction Records ===\n",
      ""
    );
    std::string const receipt_string = std::format("{:>30}", "Receipt");
    std::string const book_id_string = std::format("{:>30}", "Receipt");
    std::cout << std::format(
      Transaction::fmt_string,
      receipt_string, "User name", book_id_string, "Date", "Type"
    ) << '\n';
    std::cout << std::format("{:->130}\n", "");

    std::vector<std::reference_wrapper<Transaction const>> transactions(this->m_transactions.cbegin(), this->m_transactions.cend());
    Algorithms::selection_sort(std::span{transactions}, {}, &Transaction::get_receipt_number);

    for (Transaction const& trans : transactions) {
      std::cout << std::format(
        Transaction::fmt_string,
        trans.get_receipt_number(),
        trans.get_user_id(),
        trans.get_book_id(),
        trans.get_date(),
        trans.has_returned() ? "Return" : "Borrow"
      ) << '\n';
    }
  }

  auto book_to_modify(Book::id_type book_id) noexcept -> std::expected<std::vector<Book>::iterator, std::string>
  {
    using namespace std::string_literals;
    if (!m_curr_user || !this->m_curr_user->is_admin()) {
      return std::unexpected("Unauthorized access"s);
    }
    size_t const index = Algorithms::binary_search(std::span{std::as_const(m_books)}, book_id, {}, &Book::get_id);
    using difference_type = decltype(m_books)::difference_type;
    auto const book_it = m_books.begin() + static_cast<difference_type>(index);
    if (book_it == m_books.end()) {
      return std::unexpected("Book not found"s);
    }
    if (!book_it->is_available()) {
      return std::unexpected("Book is not available"s);
    }
    return book_it;
  }

  // User Functions
  auto search_book_by_id(Book::id_type book_id) noexcept -> Book*
  {
    // Perform sort before searching
    Algorithms::quick_sort(std::span{m_books}, {}, &Book::get_id);
    size_t const index = Algorithms::binary_search(std::span{std::as_const(m_books)}, book_id, {}, &Book::get_id);
    using difference_type = decltype(this->m_books)::difference_type;
    auto const book_iter = this->m_books.begin() + static_cast<difference_type>(index);
    if (book_iter != this->m_books.end()) {
      return &(*book_iter);
    }
    return nullptr;
  }

  auto search_book_by_title(Book::string_type const& book_title) noexcept -> Book*
  {
    // Perform sort before searching
    std::vector<std::reference_wrapper<Book const>> books_ref(m_books.cbegin(), m_books.cend());
    Algorithms::quick_sort(std::span{books_ref}, {}, &Book::get_title);
    size_t const index = Algorithms::binary_search(std::span{std::as_const(books_ref)}, book_title, {}, &Book::get_title);
    using difference_type = decltype(books_ref)::difference_type;
    auto const book_iter = books_ref.begin() + static_cast<difference_type>(index);
    if (book_iter != books_ref.end()) {
      return const_cast<Book*>(&(book_iter->get()));
    }
    return nullptr;
  }

  auto borrow_book(Book::id_type book_id, User::string_view user_id) -> std::expected<std::reference_wrapper<Transaction const>, std::string>
  {
    size_t const index = Algorithms::binary_search(std::span{std::as_const(m_books)}, book_id, {}, &Book::get_id);
    using difference_type = decltype(this->m_books)::difference_type;
    auto const book_iter = this->m_books.begin() + static_cast<difference_type>(index);
    if (book_iter == this->m_books.end())
      return std::unexpected{"No such book"};
    if (!book_iter->is_available())
      return std::unexpected{"Book is not available"};

    book_iter->set_availability(false);
    using receipt_id_type = Transaction::receipt_id_type;
    auto receipt_id = receipt_id_type{receipt_id_type::gen_t{}};
    m_transactions.emplace_back(receipt_id, User::string_type(user_id), book_id, false);
    save_books();
    save_transactions();
    return this->m_transactions.back();
  }

  auto return_book(Book::id_type book_id, User::string_view user_id) -> std::expected<std::reference_wrapper<Transaction const>, std::string>
  {
    size_t const index = Algorithms::binary_search(std::span{std::as_const(m_books)}, book_id, {}, &Book::get_id);
    using difference_type = decltype(this->m_books)::difference_type;
    auto const book_iter = this->m_books.begin() + static_cast<difference_type>(index);
    if (book_iter == this->m_books.end())
      return std::unexpected{"No such book"};
    if (book_iter->is_available())
      return std::unexpected{"Book is not borrowed"};

    book_iter->set_availability(true);
    using receipt_id_type = Transaction::receipt_id_type;
    auto receipt_id = receipt_id_type{receipt_id_type::gen_t{}};
    m_transactions.emplace_back(receipt_id, User::string_type(user_id), book_id, true);
    save_books();
    save_transactions();
    return this->m_transactions.back();
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
          "Receipt: {}\n"
          "   Book: {}\n"
          "   Date: {}\n"
          "   Type: {}\n",
          trans.get_receipt_number(),
          trans.get_book_id(),
          trans.get_date(),
          trans.has_returned() ? "Return" : "Borrow"
        );
      }
    }
  }

}; // class LibrarySystem

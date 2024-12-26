#include <algorithm>
#include <ranges>
#include <iostream>
#include <print>

import LibrarySystem;
import IO;

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
    while (true) {
      try { input(choice_base); break; }
      catch (LibraryException const&) {
        (void)input.reset().prompt("Invalid input. Please enter a number: ");
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
    input.prompt("Enter book ID to search: ").get(whitespace, book_id);
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
    input.prompt("Enter Book ID to edit: ").get(whitespace, book_id);
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
    input.prompt("Enter Book ID to delete: ").get(whitespace, book_id);
    if (system.remove_book(book_id)) {
      std::cout << "Book removed successfully!\n";
    } else {
      std::cout << "Book not found.\n";
    }
  };
  auto const view_user_transactions = [&system, &input]
  {
    User::string_type user_id;
    input.prompt("Enter User name to search: ").get(getline_t{user_id});
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
        system.view_all_books();
        break;
      case SORT_BY_ID:
        system.sort_and_view_books("m_id");
        break;
      case SORT_BY_TITLE:
        system.sort_and_view_books("m_title");
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
    while (true) {
      try { input(choice_base); break; }
      catch (LibraryException const&) {
        (void)input.reset().prompt("Invalid input. Please enter a number: ");
      }
    }
    choice = static_cast<UserChoice>(choice_base);
  };
  auto const search_book = [&system, &input]
  {
    Book::id_type book_id{};
    input.prompt("Enter book ID to search: ").get(whitespace, book_id);
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
    auto const current_user = system.current_user();
    [[assume(current_user != nullptr)]]; // Assume that the current user is not null

    Book::id_type book_id{};
    while (true) {
      try {
        input.prompt("Enter Book ID (Enter ESC to end): ").get(whitespace, ESC, book_id);
        bool const borrowed = system.borrow_book(book_id, current_user->get_username());
        if (borrowed) {
          std::cout << "Book borrowed successfully!\n";
        } else {
          std::cout << "Failed to borrow book. Book may not exist or is already borrowed.\n";
          break;
        }
      } catch (ESC_pressed const&) {
        (void)input.reset();
        break;
      }
    }
  };
  auto const view_history = [&system]
  {
    system.view_transactions_of(system.current_user()->get_username());
  };
  auto const return_book = [&system, &input]
  {
    auto const current_user = system.current_user();
    [[assume(current_user != nullptr)]]; // Assume that the current user is not null

    Book::id_type book_id{};
    while (true) {
      try {
        input.prompt("Enter Book ID (Enter ESC to end): ").get(whitespace, ESC, book_id);
        bool const returned = system.return_book(book_id, current_user->get_username());
        if (returned) {
          std::cout << "Book returned successfully!\n";
        } else {
          std::cout << "Failed to return book. Book may not exist or is not borrowed by you.\n";
          break;
        }
      } catch (ESC_pressed const&) {
        (void)input.reset();
        break;
      }
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
      (void)input.reset();
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
    while (true) {
      try{ input(choice); break; }
      catch (LibraryException const&) {
        (void)input.reset().prompt("Invalid input. Please enter a number: ");
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

[[maybe_unused]] static auto test_my_quick_sort() -> int __asm__("test_my_quick_sort");
static auto test_my_quick_sort() -> int
{
  std::vector<int> vec{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
  std::cout << "Before sorting: ";
  for (auto const& i : vec) {
    std::cout << i << ' ';
  }
  std::cout << '\n';
  Algorithms::quick_sort(std::span{vec});
  std::cout << "After sorting: ";
  for (auto const& i : vec) {
    std::cout << i << ' ';
  }
  std::cout << '\n';
  std::ranges::is_sorted(vec) ? std::cout << "Sorted\n" : std::cout << "Not sorted\n";
  return 0;
}

[[maybe_unused]] static auto test_my_binary_search() -> int __asm__("test_my_binary_search");
static auto test_my_binary_search() -> int
{
  std::vector<int> const vec{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  // std::cout << "Searching for 5: " << Algorithms::binary_search(std::span{vec}, 5) << '\n';
  // std::cout << "Searching for 11: " << Algorithms::binary_search(std::span{vec}, 11) << '\n';
  auto const search_5_result = Algorithms::binary_search(std::span{vec}, 5);
  std::println(
    "Searching for 5: {}",
    search_5_result != size_t(-1) ? std::format("Found at index {}", search_5_result) : "Not found"
  );
  auto const search_11_result = Algorithms::binary_search(std::span{vec}, 11);
  std::println(
    "Searching for 11: {}",
    search_11_result != size_t(-1) ? std::format("Found at index {}", search_11_result) : "Not found"
  );
  return 0;
}

#if !defined(TEST) && defined(TEST_UUID)
#define TEST TEST_UUID
__attribute__((alias("test_uuid")))
auto main() -> int;
#endif // TEST_UUID

#if !defined(TEST) && defined(TEST_QUICK_SORT)
#define TEST TEST_QUICK_SORT
__attribute__((alias("test_my_quick_sort")))
auto main() -> int;
#endif // TEST_QUICK_SORT

#if !defined(TEST) && defined(TEST_BINARY_SEARCH)
#define TEST TEST_BINARY_SEARCH
__attribute__((alias("test_my_binary_search")))
auto main() -> int;
#endif // TEST_BINARY_SEARCH

#if !defined(TEST)
__attribute__((alias("project_main")))
auto main() -> int;
#endif // !TEST

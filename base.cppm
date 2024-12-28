module;
#include <format>
#include <stdexcept>
#include <source_location>
#include <stacktrace>
export module base;

// Error Handling
export class LibraryException
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

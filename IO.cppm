module;
#include <utility>
#include <string>
#include <string_view>
#include <iostream>
export module IO;
import base;

export class getline_t
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
}; // class getline_t

class whitespace_t
{
  template <typename CharT, typename Traits>
  friend auto operator>>(std::basic_istream<CharT, Traits>& is, whitespace_t) -> std::basic_istream<CharT, Traits>&
  {
    is >> std::ws;
    return is;
  }
};
export constexpr inline whitespace_t whitespace{};

export class ESC_pressed
  : public LibraryException
{
public:
  ESC_pressed()
    : LibraryException("ESC key entered")
  {}
  ESC_pressed(const ESC_pressed&) = default;
  ESC_pressed(ESC_pressed&&) noexcept = default;
  auto operator=(const ESC_pressed&) -> ESC_pressed& = default;
  auto operator=(ESC_pressed&&) noexcept -> ESC_pressed& = default;
  virtual ~ESC_pressed() noexcept = default;
};

class ESC_t
{
  template <typename CharT, typename Traits>
  friend auto operator>>(std::basic_istream<CharT, Traits>& is, ESC_t) -> std::basic_istream<CharT, Traits>&
  {
    char esc = '\0';
    is.read(&esc, 1);
    if (esc == '\e')
      throw ESC_pressed();
    is.unget();
    return is;
  }
};
export constexpr inline ESC_t ESC{};

export class input_t
{
  std::reference_wrapper<std::istream> is;

public:
  explicit input_t(std::istream& is) noexcept
    : is(std::ref(is))
  { is.exceptions(std::ios_base::eofbit); }
  template <typename... Ts>
  auto operator()(Ts&&... args) const -> decltype(auto)
  {
    decltype(auto) input = (is.get() >> ... >> std::forward<Ts>(args));
    if (!input.good()) {
      throw LibraryException("Invalid input");
    }
    return (*this);
  }

  [[nodiscard]] auto prompt(std::string_view prompt) const -> decltype(auto)
  {
    std::cout << prompt;
    return (*this);
  }

  template <typename... Ts>
  auto get(Ts&&... args) const -> decltype(auto)
  {
    return (*this)(std::forward<Ts>(args)...);
  }

  [[nodiscard]] auto stream() const -> decltype(auto) { return is.get(); }

  [[nodiscard]] auto reset() const -> decltype(auto)
  {
    is.get().clear();
    is.get().ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return (*this);
  }
}; // class input_t

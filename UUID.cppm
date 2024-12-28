module;
#include <array>
#include <format>
#include <istream>
#ifdef linux
#include <uuid/uuid.h>
#elifdef _WIN32
#include <rpc.h>
#endif // _WIN32
export module UUID;

export namespace UUID_ns
{
class UUID
{
  friend std::formatter<::UUID_ns::UUID>;
private:
#ifdef linux
  using native_uuid_t = ::uuid_t;
#elifdef _WIN32
  using native_uuid_t = ::UUID;
#endif
  static constexpr size_t s_alignment = 16;
  static constexpr size_t s_size = sizeof(::uuid_t);
  alignas(s_alignment) std::array<uint8_t, s_size> m_uuid{};
  [[nodiscard]] static auto generate() noexcept -> std::array<uint8_t, s_size>
  {
    std::array<uint8_t, s_alignment> uuid{};
  #ifdef linux
    ::uuid_generate_random(uuid.data());
  #elifdef _WIN32
    ::UuidCreate(reinterpret_cast<native_uuid_t*>(uuid.data()));
  #endif
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

} // namespace UUID_ns

export template<>
struct std::formatter<::UUID_ns::UUID>
{
  template<typename ParseContext>
  constexpr static auto parse(ParseContext& ctx) noexcept { return ctx.begin(); }
  template<typename FormatContext>
  static auto format(UUID_ns::UUID const& uuid, FormatContext& ctx) noexcept -> decltype(auto)
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
    return std::copy_n(buffer.data(), uuid_width, ctx.out());
  }
};

namespace UUID_ns
{

auto operator<<(std::ostream& os, UUID_ns::UUID const& uuid) -> std::ostream&
{ return os << std::format("{}", uuid); }
auto operator>>(std::istream& is, UUID_ns::UUID& uuid) -> std::istream&
{
  static constexpr size_t buffer_size = 48;
  static constexpr size_t uuid_width = 36;
  std::array<char, buffer_size> buffer{};
  is.read(buffer.data(), uuid_width);
  if (is.gcount() != uuid_width) {
    // std::cerr << __LINE__ << ": " << __func__ << ": " << __FILE__ << ": " << "Error reading UUID\n";
    is.setstate(std::ios_base::failbit);
    return is;
  }
  if (buffer[8] != '-' || buffer[13] != '-' || buffer[18] != '-' || buffer[23] != '-') {
    // std::cerr << __LINE__ << ": " << __func__ << ": " << __FILE__ << ": " << "Error reading UUID\n";
    is.setstate(std::ios_base::failbit);
    return is;
  }
  for (auto it = buffer.data(); auto& value : uuid.m_uuid) {
    if (*it == '-') std::advance(it, 1);
    if (
      auto const [p, ec] = std::from_chars(it, std::next(it, 2), value, 16);
      ec != std::errc{}
    ) {
      // std::cerr << __LINE__ << ": " << __func__ << ": " << __FILE__ << ": " << "Error reading UUID\n";
      is.setstate(std::ios_base::failbit);
      return is;
    }
    std::advance(it, 2);
  }
  return is;
}

} // namespace UUID_ns

module;
#include <vector>
#include <span>
#include <functional>
#include <utility>
export module Algorithms;

export namespace Algorithms
{
// Merge Sort
struct merge_sort_fn
{
  template <typename T>
  static void operator()(std::vector<T>& book_list, size_t left, size_t right) noexcept
  {
    if (left >= right) return;

    size_t const mid = left + (right - left) / 2;
    merge_sort_fn{}(book_list, left, mid);
    merge_sort_fn{}(book_list, mid + 1, right);

    std::vector<T> temp(right - left + 1);
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
};
constexpr inline merge_sort_fn merge_sort{};
//Quick Sort
struct quick_sort_fn
{
  template <
    typename Element,
    typename Proj = std::identity,
    typename Comp = std::ranges::less
  > requires std::sortable<std::ranges::iterator_t<std::span<Element>>, Comp, Proj>
  static constexpr void operator()(
    std::span<Element> elements,
    Comp comp = {},
    Proj proj = {}
  )
  {
    auto const size = elements.size();
    if (size <= 1) return;
    static constexpr auto partition =
    []<typename T>(std::span<T> elements, Comp comp, Proj proj) static -> size_t
    {
      /* select pivot index */
      size_t const pivot_index = elements.size() / 2;
      /* Move the pivot value to the begin */
      std::ranges::swap(elements[0], elements[pivot_index]);
      auto const &pivot_value = elements[0];
      /* Partition the elements */
      size_t result = 1;
      for (size_t i = 1; i < elements.size(); ++i) {
        /*
        E.g. Suppose that it's equivalent with `if (value < p)`, then:
        `comp` is `operator<`;
        `proj` is `std::identity`;
        `value` is `std::invoke(proj, elements[i])`;
        `p` is `std::invoke(proj, pivot_value)`.
        proj(value) => invoke(proj, value)
        */
        if (std::invoke(comp, std::invoke(proj, elements[i]), std::invoke(proj, pivot_value))) {
          std::ranges::swap(elements[i], elements[result]);
          ++result;
        }
      }
      std::ranges::swap(elements[0], elements[--result]);
      return result;
    };
    auto const partition_index = partition(elements, comp, proj);
    quick_sort_fn{}(elements.first(partition_index), comp, proj);
    quick_sort_fn{}(elements.subspan(partition_index + 1), comp, proj);
  }
};
constexpr inline quick_sort_fn quick_sort{};
// Selection Sort
struct selection_sort_fn
{
  template <typename T>
  static void operator()(std::vector<T>& book_list) noexcept
  {
    size_t const n = book_list.size();
    for (size_t i = 0; i < n - 1; i++) {
      size_t min_idx = i;
      for (size_t j = i + 1; j < n; j++) {
        if (book_list[j].get_id() < book_list[min_idx].get_id()) {
          min_idx = j;
        }
      }
      if (min_idx != i) {
        std::swap(book_list[i], book_list[min_idx]);
      }
    }
  }
};
constexpr inline selection_sort_fn selection_sort{};
// Bubble Sort
struct bubble_sort_fn
{
  template <typename T>
  static void operator()(std::vector<T>& book_list) noexcept
  {
    size_t const n = book_list.size();
    for (size_t i = 0; i < n - 1; i++) {
      for (size_t j = 0; j < n - i - 1; j++) {
        if (book_list[j].get_title() > book_list[j + 1].get_title())
          std::swap(book_list[j], book_list[j + 1]);
      }
    }
  }
};
constexpr inline bubble_sort_fn bubble_sort{};
// Searching Algorithms
struct binary_search_fn
{
  template <
    typename Element,
    typename Key,
    typename Proj = std::identity,
    std::indirect_strict_weak_order<
      Key const*,
      std::projected<
        std::ranges::iterator_t<std::span<Element const>>,
        Proj
      >
    > Comp = std::ranges::less
  >
  static constexpr auto operator()(
    std::span<Element const> elements,
    Key const& key,
    Comp comp = {},
    Proj proj = {}
  ) -> std::size_t
  {
    auto const end = elements.end();
    auto first = elements.begin();
    auto last = elements.end();
    while (first < last) {
      auto const middle = first + (last - first) / 2;
      if (std::invoke(comp, std::invoke(proj, *middle), key)) {
        first = middle + 1;
      } else {
        last = middle;
      }
    }
    return first != end && std::invoke(proj, *first) == key ?
      static_cast<std::size_t>(first - elements.begin()) :
      -1;
  }
};
constexpr inline binary_search_fn binary_search{};
struct linear_search_fn
{
  template <typename T>
  static auto operator()(std::span<T> elements, T const& key) -> size_t
  {
    for (size_t i = 0; i < elements.size(); ++i) {
      if (elements[i] == key) {
        return i;
      }
    }
    return -1;
  }
};
constexpr inline linear_search_fn linear_search{};
} // namespace Algorithms

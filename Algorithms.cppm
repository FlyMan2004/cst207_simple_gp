module;
#include <vector>
#include <span>
#include <iterator>
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
    if (right - left <= 1) return;

    size_t const mid = left + (right - left) / 2;
    merge_sort_fn{}(book_list, left, mid);
    merge_sort_fn{}(book_list, mid, right);

    std::vector<T> temp(right - left);
    size_t i = left, j = mid, k = 0;

    while (i < mid && j < right) {
      if (book_list.at(i).get_id() <= book_list.at(j).get_id())
        temp.at(k++) = std::move(book_list.at(i++));
      else
        temp.at(k++) = std::move(book_list.at(j++));
    }

    while (i < mid) temp.at(k++) = std::move(book_list.at(i++));
    while (j < right) temp.at(k++) = std::move(book_list.at(j++));

    for (i = 0; i < temp.size(); i++)
      book_list.at(left + i) = std::move(temp.at(i));
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
    [](std::span<Element> elements, Comp comp, Proj proj) static -> size_t
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
  template <
    typename T,
    typename Proj = std::identity,
    typename Comp = std::ranges::less
  > requires std::sortable<std::ranges::iterator_t<std::span<T>>, Comp, Proj>
  static constexpr void operator()(
    std::span<T> elements,
    Comp comp = {},
    Proj proj = {}
  )
  {
    for (size_t i = 0; i < elements.size() - 1; i++) {
      size_t min_idx = i;
      for (size_t j = i + 1; j < elements.size(); j++) {
        if (std::invoke(comp, std::invoke(proj, elements[j]), std::invoke(proj, elements[min_idx]))) {
          min_idx = j;
        }
      }
      if (min_idx != i) {
        std::ranges::swap(elements[i], elements[min_idx]);
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
      bool swapped = false;
      for (size_t j = 0; j < n - i - 1; j++) {
        if (book_list.at(j).get_title() > book_list.at(j + 1).get_title()) {
          std::swap(book_list[j], book_list[j + 1]);
          swapped = true;
        }
      }
      if (!swapped) break;
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
      std::ranges::distance(elements);
  }
};
constexpr inline binary_search_fn binary_search{};
struct linear_search_fn
{
  template <typename T>
  static auto operator()(std::span<T> elements, T const& key) -> size_t
  {
    for (size_t i = 0; i < elements.size(); ++i) {
      if (elements.at(i) == key) {
        return i;
      }
    }
    return std::ranges::distance(elements);
  }
};
constexpr inline linear_search_fn linear_search{};
} // namespace Algorithms

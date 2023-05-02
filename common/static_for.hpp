#pragma once

#include <type_traits>
#include <concepts>

namespace GBA::common {
	namespace detail {
		template <int Begin, int End, typename Func, typename Param>
		struct StaticForHelper {
			static constexpr auto execute(Func&& function, Param&& param) {
				if constexpr (Begin <= End) {
					auto res = function(Begin, param);

					return StaticForHelper<Begin + 1, End, Func, decltype(res)>::execute(
						std::forward<Func>(function),
						std::forward<decltype(res)>(res)
					);
				}
				else {
					return std::forward<Param>(param);
				}
			}
		};

		template <int End, typename Func, typename Param>
		struct StaticForHelper<End, End, Func, Param> {
			static constexpr auto execute(Func&&, Param&& param) -> decltype(param)  {
				return std::forward<Param>( param );
			}
		};

		template <int Begin, int End, int BlockSize, typename Func, typename Param>
		struct StaticForByBlock {
			static_assert(Begin <= End);
			static_assert(BlockSize <= 256);

			static constexpr auto execute(Func&& function, Param&& param) {
				if constexpr (Begin + BlockSize >= End) {
					return StaticForHelper<Begin, End, Func, Param>::execute(
						std::forward<Func>(function),
						std::forward<Param>(param)
					);
				}
				else {
					auto res = StaticForHelper<Begin, Begin + BlockSize, Func, Param>::execute(
						std::forward<Func>(function),
						std::forward<Param>(param)
					);

					return StaticForByBlock<Begin + BlockSize, End, BlockSize, Func, decltype(res)>::execute(
						std::forward<Func>(function),
						std::forward<decltype(res)>(res)
					);
				}
			}
		};
	}
	
	template <int Begin, int End, typename Func, typename Param>
	constexpr auto static_for(Func&& function, Param&& param) {
		static_assert(Begin <= End);

		return detail::StaticForByBlock<Begin, End, 128, Func, Param>::execute(
			std::forward<Func>(function),
			std::forward<Param>(param)
		);
	}
}
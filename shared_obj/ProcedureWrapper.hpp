#pragma once

#include <typeinfo>
#include <vector>
#include <any>

namespace GBA::shared_object {
	enum class CallConvetion {
		STCALL,
		THISCALL,
		_CDECL
	};

	namespace detail {
		template <typename FunctionType,
			CallConvetion convetion,
			typename ReturnType,
			typename... Args>
		struct templated_wrapper_impl {};

		template <typename ReturnType, typename FunctionType, typename... Args>
		struct templated_wrapper_impl<FunctionType, CallConvetion::STCALL, ReturnType, Args...>
		{
			using RealFunctionType = ReturnType(__stdcall*)(Args...);

			template <std::size_t... I>
			static constexpr ReturnType call(void* fptr, std::vector<std::any>& args, std::index_sequence<I...> seq) {
				return ((RealFunctionType)fptr)((std::any_cast<Args>(args[I]))...);
			}
		};

		template <typename ReturnType, typename FunctionType, typename... Args>
		struct templated_wrapper_impl<FunctionType, CallConvetion::_CDECL, ReturnType, Args...>
		{
			using RealFunctionType = ReturnType(__cdecl*)(Args...);

			template <std::size_t... I>
			static constexpr ReturnType call(void* fptr, std::vector<std::any>& args, std::index_sequence<I...> seq) {
				return ((RealFunctionType)fptr)((std::any_cast<Args>(args[I]))...);
			}
		};

		template <typename, CallConvetion>
		struct templated_wrapper {};

		template <CallConvetion convetion, typename ReturnType, typename... Args>
		struct templated_wrapper<ReturnType(*)(Args...), convetion> {
			static constexpr ReturnType call(void* fptr, std::vector<std::any> args) {
				return 
					templated_wrapper_impl<ReturnType(*)(Args...), CallConvetion::STCALL,
					ReturnType, Args...>::template call(fptr, args, std::make_index_sequence<sizeof...(Args)>{});
			}
		};
	}

	class FunctionWrapper {
	public :
		FunctionWrapper(void* ptr) : 
			fptr(ptr) {}

		template <typename T, typename... Args>
		T call(Args&&... args) {
			std::vector<std::any> argument_vector{ args... };

			if constexpr (std::is_same_v<T, void>) {
				call_impl(argument_vector);
			}
			else {
				return std::forward<T>(std::any_cast<T>(
					call_impl(argument_vector)
				));
			}
		}

		void* get_pointer() {
			return fptr;
		}

	protected :
		virtual std::any call_impl(std::vector<std::any>& vec) = 0;
		
		void* fptr;
	};

	template <typename FType, CallConvetion convention>
	class FunctionWrapperImpl : public FunctionWrapper {
	public :
		FunctionWrapperImpl(void* ptr) : FunctionWrapper(ptr) {}

	protected :
		std::any call_impl(std::vector<std::any>& vec) override {
			using ret_type = decltype(detail::templated_wrapper<FType, convention>::call(
				fptr, vec
			));

			if constexpr (std::is_same_v<ret_type, void>) {
				detail::templated_wrapper<FType, convention>::call(fptr, vec);
				return std::any{};
			}
			else {
				return std::any{
					detail::templated_wrapper<FType, convention>::call(
						fptr, vec
					)
				};
			}
		}
	};
}
#pragma once
namespace Gawr::ECS {
	namespace internal {
		template<typename T, typename ... Ts>
		struct Contains { static constexpr bool value = (std::is_same_v<T, Ts> || ...); };
	}

	template<typename ... Ts>
	struct Select { 
	private:
		using T0 = std::tuple_element_t<0, std::tuple<Ts...>>;
	public:
		using Return_T = std::conditional_t<
			std::tuple_size_v<std::tuple<Ts...>> == 1, 
				std::conditional_t<std::is_same_v<T0, Entity>, T0, T0&>,
				decltype(std::tuple_cat(std::declval<std::conditional_t<std::is_same_v<Ts, Entity>, std::tuple<Entity>, std::tuple<Ts&>>>()...))>;

		static_assert(!(std::is_empty_v<Ts> || ...), "Empty types are not stored and cannot be retrieved.");

		template<typename Pip_T>
		static Return_T retrieve(Pip_T& pipeline, Entity e) {
			if constexpr (sizeof...(Ts) == 1)
			{
				if constexpr (std::is_same_v<T0, Entity>)
					return e;
				else
					return pipeline.pool<T0>().getComponent(e);
			}
			else 
			{
				return std::tuple_cat([&]<typename U>()->auto
				{
					if constexpr (std::is_same_v<U, Entity>)
						return std::tuple(e);
					else
						return std::tuple<U&>{ pipeline.pool<U>().getComponent(e) };
				}.operator()<Ts>()...);
			}
		}
	};
		
	template<typename T>
	struct From { 
		using type = const T;
	};

	template<typename ... Ts>
	struct AllOf {
		template<typename Pip_T>
		static bool match(Pip_T& pip, Entity e) {
			if constexpr (sizeof...(Ts) == 0) return true;
			return (pip.pool<const Ts>().contains(e) && ...);
		}
	};

	template<typename ... Ts>
	struct NoneOf { 
		template<typename Pip_T>
		static bool match(Pip_T& pip, Entity e) {
			if constexpr (sizeof...(Ts) == 0) return true;
			return !(pip.pool<const Ts>().contains(e) || ...);
		}
	};

	template<typename AllOf, typename NoneOf = NoneOf<>>
	struct Where;

	template<typename ... AllOfArgs, typename ... NoneOfArgs>
	struct Where<AllOf<AllOfArgs...>, NoneOf<NoneOfArgs...>> {
		static_assert((!Contains<NoneOfArgs, AllOfArgs...>::value && ...), "NoneOf Filter intersects with AllOf Filter");
		template<typename Pip_T>
		static bool match(Pip_T& pip, Entity e) {
			return AllOf<AllOfArgs...>::match(pip, e) && NoneOf<NoneOfArgs...>::match(pip, e);
		}
	};

	namespace internal {
		template<typename Select_T>
		struct DefaultFrom;

		template<typename T>
		struct DefaultFrom<Select<T>> {
			using type = From<std::remove_const_t<T>>;
		};

		template<typename T0, typename T1, typename ... Ts>
		struct DefaultFrom<Select<T0, T1, Ts...>> {
			using type = From<std::conditional_t<std::is_same_v<T0, Entity>, std::remove_const_t<T1>, std::remove_const_t<T0>>>;
		};

		template<typename Select_T, typename From_T, typename result = AllOf<>>
		struct DefaultAllOf;

		template<typename From_T, typename ... Res_Ts>
		struct DefaultAllOf<Select<>, From_T, AllOf<Res_Ts...>> {
			using type = AllOf<Res_Ts...>;
		};

		template<typename Select_Arg_T, typename ... Select_Arg_Ts, typename From_Arg_T, typename ... Res_Ts>
		struct DefaultAllOf<Select<Select_Arg_T, Select_Arg_Ts...>, From<From_Arg_T>, AllOf<Res_Ts...>> {
		private:
			using curr = std::remove_const_t<Select_Arg_T>;
		public:
			using type = std::conditional_t<std::is_same_v<Entity, curr> || std::is_same_v<From_Arg_T, curr>,
				DefaultAllOf<Select<Select_Arg_Ts...>, From<From_Arg_T>, AllOf<Res_Ts...>>,
				DefaultAllOf<Select<Select_Arg_Ts...>, From<From_Arg_T>, AllOf<curr, Res_Ts...>>>::type;
		};

		template<typename Select_T, typename From_T>
		struct DefaultWhere {
			using type = Where<typename DefaultAllOf<Select_T, From_T>::type, NoneOf<>>;
		};
	}
}

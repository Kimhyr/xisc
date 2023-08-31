export module xisc.parser;

import xisc.syntax.token;

import std;

namespace xisc
{

namespace detail
{

template<auto T, auto... Ts>
struct find : std::false_type {};

template<auto T, auto... Ts>
struct find<T, T, Ts...> : std::true_type {};

template<auto T>
struct find<T> : std::false_type {};

template<auto T, auto... Ts>
constexpr auto find_v = find<T, Ts...>::value;

template<auto... Toks>
struct match
{
	static constexpr auto value = [](auto tok) { return false; };
};

template<auto Tok, auto... Toks>
struct match<Tok, Toks...>
{
	static constexpr auto value = [](auto tok) { return tok == Tok ? true : match<Toks...>::value(tok); };
};

template<auto ...Toks>
constexpr auto match_v(auto tok) -> bool
{
	return match<Toks...>::value(tok);
}

}

template<token_type ...Toks>
using token_sequence = std::integer_sequence<token_type, Toks...>;

export class parser;

template<typename T>
concept parsable =
	requires (T& s, parser& p)
	{
		{ T::parse(s, p) } -> std::same_as<parser&>;
	};

class parser
{
	enum class option_t
	{
		on_success,
		on_failure,
		dont_report,
		ignore_failure
	};

	enum class failure_t
	{
		none,
		unexpected_token
	};

public:
	using resolution_t = void (*)();

private:
	static constexpr resolution_t do_nothing = []{};

public:
	parser() = default;
	
	template<typename T, option_t ...Opts>
	auto parse(
		T& out,
		resolution_t on_success = do_nothing,
		resolution_t on_failure = do_nothing
	) -> parser&
	{
		return T::parse(out, *this);
	}

	auto consume() -> parser&
	{
		return *this;
	}

	template<token_type Tok, option_t ...Opts>
	auto expect(
		resolution_t on_success = do_nothing,
		resolution_t on_failure = do_nothing
	) -> parser&
	{
		return this->if_success_else_failure<
			failure_t::unexpected_token,
			Tok, Opts...
		>(
			[&]{ return this->consume().token() == Tok; },
			on_success, on_failure
		);
	}

	template<failure_t Fail, auto ...Vs, typename ...Args>
	auto report(Args... args) -> parser&
	{
		return *this;
	}

	[[nodiscard]]
	auto token() -> token const&
	{
		return this->m_token;
	}

private:
	xisc::token m_token;
	
	template<
		failure_t Fail = failure_t::none,
		auto... Args
	>
	auto if_success_else_failure(
		bool (*f)(),
		resolution_t on_success,
		resolution_t on_failure
	) -> parser&
	{
		if (f()) [[unlikely]] {
			if constexpr(detail::find_v<option_t::on_success, Args...>)
				on_success();
		} else if constexpr(!detail::find_v<option_t::ignore_failure, Args...>) {
			if constexpr(!detail::find_v<option_t::dont_report, Args...>)
				this->report<Fail, Args...>();
			if constexpr(detail::find_v<option_t::on_failure, Args...>) {
				if constexpr(detail::find_v<option_t::on_success, Args...>)
					on_failure();
				else on_success();
			}
		}
		return *this;
	}
};

}

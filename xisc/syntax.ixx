export module xisc.syntax;
import xisc.parser;

namespace xisc
{

struct program
{
	static auto parse(program& self, parser& parser) -> xisc::parser&
	{
		return parser;
	}
};

}

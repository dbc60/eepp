#ifndef EE_UI_DOC_SYNTAXTOKENIZER_HPP
#define EE_UI_DOC_SYNTAXTOKENIZER_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>
#include <eepp/ui/doc/syntaxdefinition.hpp>
#include <string>

using namespace EE::Graphics;

namespace EE { namespace UI { namespace Doc {

struct EE_API SyntaxToken {
	std::string type;
	std::string text;
};

#define SYNTAX_TOKENIZER_STATE_NONE ( 0 )

struct SyntaxState {
	const SyntaxDefinition* currentSyntax{ nullptr };
	const SyntaxPattern* subsyntaxInfo{ nullptr };
	Uint32 currentPatternIdx{ 0 };
	Uint32 currentLevel{ 0 };
};

class EE_API SyntaxTokenizer {
  public:
	static std::pair<std::vector<SyntaxToken>, Uint32> tokenize( const SyntaxDefinition& syntax,
																 const std::string& text,
																 const Uint32& state,
																 const size_t& startIndex = 0 );

	static Text& tokenizeText( const SyntaxDefinition& syntax, const SyntaxColorScheme& colorScheme,
							   Text& text, const size_t& startIndex = 0,
							   const size_t& endIndex = 0xFFFFFFFF );

	static SyntaxState retrieveSyntaxState( const SyntaxDefinition& syntax, const Uint32& state );
};

}}} // namespace EE::UI::Doc

#endif // EE_UI_DOC_SYNTAXTOKENIZER_HPP

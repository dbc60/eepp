#include <algorithm>
#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <iostream>

namespace EE { namespace UI { namespace CSS {

StyleSheet::StyleSheet() {}

template <class T> inline void HashCombine( std::size_t& seed, const T& v ) {
	seed ^= String::hash( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

size_t StyleSheet::NodeHash( const std::string& tag, const std::string& id ) {
	size_t seed = 0;
	if ( !tag.empty() )
		seed = String::hash( tag );
	if ( !id.empty() )
		HashCombine( seed, id );
	return seed;
}

void StyleSheet::addStyleToNodeIndex( StyleSheetStyle* style ) {
	const std::string& id = style->getSelector().getSelectorId();
	const std::string& tag = style->getSelector().getSelectorTagName();
	if ( style->hasProperties() || style->hasVariables() ) {
		size_t nodeHash = NodeHash( "*" == tag ? "" : tag, id );
		StyleSheetStyleVector& nodes = mNodeIndex[nodeHash];
		auto it = std::find( nodes.begin(), nodes.end(), style );
		if ( it == nodes.end() ) {
			nodes.push_back( style );
		} else {
			eePRINTL( "Ignored style %s", style->getSelector().getName().c_str() );
		}
	}
}

void StyleSheet::addStyle( std::shared_ptr<StyleSheetStyle> node ) {
	mNodes.push_back( node );
	addStyleToNodeIndex( node.get() );
	addMediaQueryList( node->getMediaQueryList() );
}

bool StyleSheet::isEmpty() const {
	return mNodes.empty();
}

void StyleSheet::print() {
	for ( auto& style : mNodes ) {
		std::cout << style->build();
	}
}

void StyleSheet::combineStyleSheet( const StyleSheet& styleSheet ) {
	for ( auto& style : styleSheet.getStyles() ) {
		addStyle( style );
	}

	addKeyframes( styleSheet.getKeyframes() );
}

StyleSheetStyleVector StyleSheet::getElementStyles( UIWidget* element,
													const bool& applyPseudo ) const {
	static StyleSheetStyleVector applicableNodes;
	applicableNodes.clear();

	const std::string& tag = element->getElementTag();
	const std::string& id = element->getId();

	std::array<size_t, 4> nodeHash;
	int numHashes = 2;

	nodeHash[0] = 0;
	nodeHash[1] = NodeHash( tag, "" );

	if ( !id.empty() ) {
		numHashes = 4;
		nodeHash[2] = NodeHash( "", id );
		nodeHash[3] = NodeHash( tag, id );
	}

	for ( int i = 0; i < numHashes; i++ ) {
		auto it_nodes = mNodeIndex.find( nodeHash[i] );
		if ( it_nodes != mNodeIndex.end() ) {
			const StyleSheetStyleVector& nodes = it_nodes->second;
			for ( StyleSheetStyle* node : nodes ) {
				if ( node->getSelector().select( element, applyPseudo ) ) {
					applicableNodes.push_back( node );
				}
			}
		}
	}

	return applicableNodes;
}

const std::vector<std::shared_ptr<StyleSheetStyle>>& StyleSheet::getStyles() const {
	return mNodes;
}

bool StyleSheet::updateMediaLists( const MediaFeatures& features ) {
	if ( mMediaQueryList.empty() )
		return false;

	bool updateStyles = false;

	for ( auto iter = mMediaQueryList.begin(); iter != mMediaQueryList.end(); iter++ ) {
		if ( ( *iter )->applyMediaFeatures( features ) ) {
			updateStyles = true;
			break;
		}
	}

	return updateStyles;
}

bool StyleSheet::isMediaQueryListEmpty() const {
	return mMediaQueryList.empty();
}

void StyleSheet::addMediaQueryList( MediaQueryList::ptr list ) {
	if ( list ) {
		if ( std::find( mMediaQueryList.begin(), mMediaQueryList.end(), list ) ==
			 mMediaQueryList.end() ) {
			mMediaQueryList.push_back( list );
		}
	}
}

StyleSheetStyleVector StyleSheet::getStyleSheetStyleByAtRule( const AtRuleType& atRuleType ) const {
	StyleSheetStyleVector vector;

	for ( auto& node : mNodes ) {
		if ( node->getAtRuleType() == atRuleType ) {
			vector.push_back( node.get() );
		}
	}

	return vector;
}

bool StyleSheet::isKeyframesDefined( const std::string& keyframesName ) {
	return mKeyframesMap.find( keyframesName ) != mKeyframesMap.end();
}

const KeyframesDefinition& StyleSheet::getKeyframesDefinition( const std::string& keyframesName ) {
	static KeyframesDefinition EMPTY;
	const auto& it = mKeyframesMap.find( keyframesName );
	if ( it != mKeyframesMap.end() ) {
		return it->second;
	}
	return EMPTY;
}

void StyleSheet::addKeyframes( const KeyframesDefinition& keyframes ) {
	// "none" is a reserved keyword
	if ( keyframes.getName() != "none" )
		mKeyframesMap[keyframes.getName()] = keyframes;
}

void StyleSheet::addKeyframes( const KeyframesDefinitionMap& keyframesMap ) {
	for ( auto& keyframes : keyframesMap ) {
		addKeyframes( keyframes.second );
	}
}

const KeyframesDefinitionMap& StyleSheet::getKeyframes() const {
	return mKeyframesMap;
}

}}} // namespace EE::UI::CSS

#include <eepp/graphics/fontmanager.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/stylesheetpropertyanimation.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>

using namespace EE::UI::CSS;
using namespace EE::Scene;

namespace EE { namespace UI {

UIStyle* UIStyle::New( UIWidget* widget ) {
	return eeNew( UIStyle, ( widget ) );
}

UIStyle::UIStyle( UIWidget* widget ) :
	UIState(),
	mWidget( widget ),
	mElementStyle( std::make_shared<CSS::StyleSheetStyle>() ),
	mDefinition( std::make_shared<CSS::ElementDefinition>( StyleSheetStyleVector() ) ),
	mChangingState( false ),
	mForceReapplyProperties( false ),
	mDisableAnimations( false ) {}

UIStyle::~UIStyle() {
	removeRelatedWidgets();
	unsubscribeNonCacheableStyles();
}

bool UIStyle::stateExists( const EE::Uint32& ) const {
	return true;
}

void UIStyle::setStyleSheetProperty( const StyleSheetProperty& property ) {
	std::vector<StyleSheetProperty> properties;

	if ( StyleSheetSpecification::instance()->isShorthand( property.getName() ) ) {
		properties = StyleSheetSpecification::instance()
						 ->getShorthand( property.getName() )
						 ->parse( property.getValue() );
	} else {
		properties.emplace_back( property );
	}

	for ( auto& prop : properties ) {
		mElementStyle.get()->setProperty( prop );
	}
}

void UIStyle::load() {
	unsubscribeNonCacheableStyles();

	mCacheableStyles.clear();
	mNoncacheableStyles.clear();
	mElementStyle->clearProperties();
	mVariables.clear();
	mStructurallyVolatile = false;

	UISceneNode* uiSceneNode = mWidget->getUISceneNode();

	if ( NULL != uiSceneNode ) {
		CSS::StyleSheet& styleSheet = uiSceneNode->getStyleSheet();

		if ( !styleSheet.isEmpty() ) {
			StyleSheetStyleVector styles = styleSheet.getElementStyles( mWidget );

			for ( auto& style : styles ) {
				const StyleSheetSelector& selector = style->getSelector();

				if ( selector.isCacheable() ) {
					mCacheableStyles.push_back( style );
				} else {
					mNoncacheableStyles.push_back( style );
				}

				if ( selector.isStructurallyVolatile() )
					mStructurallyVolatile = true;

				findVariables( style );
			}

			subscribeNonCacheableStyles();
		}
	}
}

void UIStyle::setStyleSheetProperties( const CSS::StyleSheetProperties& properties ) {
	for ( const auto& it : properties ) {
		setStyleSheetProperty( it.second );
	}
}

bool UIStyle::hasTransition( const std::string& propertyName ) {
	return mTransitions.find( propertyName ) != mTransitions.end() ||
		   mTransitions.find( "all" ) != mTransitions.end();
}

StyleSheetPropertyAnimation* UIStyle::getAnimation( const PropertyDefinition* propertyDef ) {
	std::vector<Action*> actions = mWidget->getActionsByTag( propertyDef->getId() );
	if ( !actions.empty() ) {
		for ( auto& action : actions ) {
			if ( action->getId() == StyleSheetPropertyAnimation::ID ) {
				StyleSheetPropertyAnimation* animation =
					static_cast<StyleSheetPropertyAnimation*>( action );
				if ( animation->getAnimationOrigin() == AnimationOrigin::Animation ) {
					return animation;
				}
			}
		}
	}
	return NULL;
}

bool UIStyle::hasAnimation( const PropertyDefinition* propertyDef ) {
	return NULL != getAnimation( propertyDef );
}

TransitionDefinition UIStyle::getTransition( const std::string& propertyName ) {
	auto propertyTransitionIt = mTransitions.find( propertyName );

	if ( propertyTransitionIt != mTransitions.end() ) {
		return propertyTransitionIt->second;
	} else if ( ( propertyTransitionIt = mTransitions.find( "all" ) ) != mTransitions.end() ) {
		return propertyTransitionIt->second;
	}

	return TransitionDefinition();
}

const bool& UIStyle::isChangingState() const {
	return mChangingState;
}

StyleSheetVariable UIStyle::getVariable( const std::string& variable ) {
	auto it = mVariables.find( String::hash( variable ) );

	if ( it != mVariables.end() ) {
		return it->second;
	} else {
		Node* parentWidget = mWidget->getParentWidget();

		if ( NULL != parentWidget ) {
			UIStyle* style = parentWidget->asType<UIWidget>()->getUIStyle();

			if ( NULL != style ) {
				return style->getVariable( variable );
			}
		}
	}

	return StyleSheetVariable();
}

bool UIStyle::getForceReapplyProperties() const {
	return mForceReapplyProperties;
}

void UIStyle::setForceReapplyProperties( bool forceReapplyProperties ) {
	mForceReapplyProperties = forceReapplyProperties;
}

bool UIStyle::getDisableAnimations() const {
	return mDisableAnimations;
}

void UIStyle::setDisableAnimations( bool disableAnimations ) {
	mDisableAnimations = disableAnimations;
}

const bool& UIStyle::isStructurallyVolatile() const {
	return mStructurallyVolatile;
}

void UIStyle::subscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.insert( widget );
}

void UIStyle::unsubscribeRelated( UIWidget* widget ) {
	mRelatedWidgets.erase( widget );
}

void UIStyle::tryApplyStyle( StyleSheetStyleVector& styles, StyleSheetStyle* style ) {
	if ( style->isMediaValid() && style->getSelector().select( mWidget ) ) {
		styles.push_back( style );
	}
}

void UIStyle::findVariables( const StyleSheetStyle* style ) {
	for ( const auto& vars : style->getVariables() ) {
		const StyleSheetVariable& variable = vars.second;
		const auto& it = mVariables.find( variable.getNameHash() );

		if ( it == mVariables.end() || variable.getSpecificity() >= it->second.getSpecificity() ) {
			mVariables[variable.getNameHash()] = variable;
		}
	}
}

void UIStyle::setVariableFromValue( StyleSheetProperty* property, const std::string& value ) {
	std::string newValue( value );
	for ( auto& var : property->getVarCache() ) {
		for ( auto& val : var.variableList ) {
			StyleSheetVariable variable( getVariable( val ) );
			if ( !variable.isEmpty() ) {
				String::replaceAll( newValue, var.definition, variable.getValue() );
				break;
			}
		}
	}
	property->setValue( newValue );
}

void UIStyle::applyVarValues( StyleSheetProperty* property ) {
	if ( property->isVarValue() ) {
		if ( NULL != property->getPropertyDefinition() &&
			 property->getPropertyDefinition()->isIndexed() ) {
			for ( size_t i = 0; i < property->getPropertyIndexCount(); i++ ) {
				StyleSheetProperty* realProperty = property->getPropertyIndexRef( i );
				setVariableFromValue( realProperty, realProperty->getValue() );
			}
		} else {
			setVariableFromValue( property, property->getValue() );
		}
	}
}

template <typename Map> bool mapEquals( Map const& lhs, Map const& rhs ) {
	// No predicate needed because there is operator== for pairs already.
	return lhs.size() == rhs.size() && std::equal( lhs.begin(), lhs.end(), rhs.begin() );
}

void UIStyle::onStateChange() {
	if ( NULL != mWidget ) {
		mChangingState = true;

		mTransitionProperties.clear();
		mAnimationProperties.clear();

		CSS::StyleSheetStyleVector newStyles;

		tryApplyStyle( newStyles, mElementStyle.get() );

		for ( auto& style : mCacheableStyles ) {
			tryApplyStyle( newStyles, style );
		}

		for ( auto& style : mNoncacheableStyles ) {
			tryApplyStyle( newStyles, style );
		}

		std::shared_ptr<ElementDefinition> prevDefinition = mDefinition;
		std::shared_ptr<ElementDefinition> newDefinition =
			std::make_shared<ElementDefinition>( newStyles );

		if ( !mapEquals( mDefinition->getProperties(), newDefinition->getProperties() ) ||
			 mForceReapplyProperties ) {
			PropertyIdSet changedProperties;

			for ( auto& prop : newDefinition->getProperties() ) {
				const StyleSheetProperty& property = prop.second;

				if ( String::startsWith( property.getName(), "transition" ) )
					mTransitionProperties.push_back( &property );
				else if ( String::startsWith( property.getName(), "animation" ) )
					mAnimationProperties.push_back( &property );
			}

			if ( mDefinition )
				changedProperties = mDefinition->getPropertyIds();

			if ( newDefinition )
				changedProperties |= newDefinition->getPropertyIds();

			if ( !newDefinition->getPropertyIds().empty() ) {
				// Remove properties that compare equal from the changed list.
				const PropertyIdSet propertiesInBothDefinitions =
					( mDefinition->getPropertyIds() & newDefinition->getPropertyIds() );

				for ( Uint32 id : propertiesInBothDefinitions ) {
					const StyleSheetProperty* p0 = mDefinition->getProperty( id );
					const StyleSheetProperty* p1 = newDefinition->getProperty( id );
					if ( nullptr != p0 && nullptr != p1 && *p0 == *p1 )
						changedProperties.erase( id );
				}

				mDefinition = newDefinition;
			}

			mForceReapplyProperties = false;

			mWidget->beginAttributesTransaction();

			updateAnimations();

			if ( !mTransitionProperties.empty() ) {
				mTransitions =
					TransitionDefinition::parseTransitionProperties( mTransitionProperties );
			}

			for ( auto prop : changedProperties ) {
				StyleSheetProperty* property = getLocalProperty( prop );

				if ( nullptr == property )
					continue;

				if ( NULL == property->getPropertyDefinition() )
					continue;

				applyVarValues( property );

				if ( property->getPropertyDefinition()->isIndexed() ) {
					for ( size_t i = 0; i < property->getPropertyIndexCount(); i++ ) {
						applyStyleSheetProperty( property->getPropertyIndex( i ), prevDefinition );
					}
				} else {
					applyStyleSheetProperty( *property, prevDefinition );
				}
			}

			mWidget->endAttributesTransaction();
		}

		for ( auto& related : mRelatedWidgets ) {
			if ( NULL != related->getUIStyle() ) {
				related->getUIStyle()->onStateChange();
			}
		}

		mChangingState = false;
	}
}

const StyleSheetProperty*
UIStyle::getStatelessStyleSheetProperty( const Uint32& propertyId ) const {
	if ( 0 != propertyId ) {
		if ( !mElementStyle->getSelector().hasPseudoClasses() ) {
			const StyleSheetProperty* property = mElementStyle->getPropertyById( propertyId );

			if ( property )
				return property;
		}

		for ( auto style : mCacheableStyles ) {
			if ( !style->getSelector().hasPseudoClasses() ) {
				const StyleSheetProperty* property = style->getPropertyById( propertyId );

				if ( property )
					return property;
			}
		}
	}

	return nullptr;
}

void UIStyle::updateState() {
	for ( int i = StateFlagCount - 1; i >= 0; i-- ) {
		if ( ( mState & getStateFlag( i ) ) == getStateFlag( i ) ) {
			if ( stateExists( getStateFlag( i ) ) ) {
				if ( mCurrentState != getStateFlag( i ) ) {
					mPreviousState = mCurrentState;
					mCurrentState = getStateFlag( i );
					break;
				}
			}
		}
	}

	onStateChange();
}

void UIStyle::subscribeNonCacheableStyles() {
	for ( auto& style : mNoncacheableStyles ) {
		std::vector<UIWidget*> elements = style->getSelector().getRelatedElements( mWidget, false );

		if ( !elements.empty() ) {
			for ( auto& element : elements ) {
				UIWidget* widget = element->asType<UIWidget>();

				if ( NULL != widget && NULL != widget->getUIStyle() ) {
					widget->getUIStyle()->subscribeRelated( mWidget );

					mSubscribedWidgets.insert( widget );
				}
			}
		}
	}
}

void UIStyle::unsubscribeNonCacheableStyles() {
	for ( auto& widget : mSubscribedWidgets ) {
		if ( NULL != widget->getUIStyle() ) {
			widget->getUIStyle()->unsubscribeRelated( mWidget );
		}
	}

	mSubscribedWidgets.clear();
}

void UIStyle::removeFromSubscribedWidgets( UIWidget* widget ) {
	mSubscribedWidgets.erase( widget );
}

void UIStyle::removeRelatedWidgets() {
	for ( auto& widget : mRelatedWidgets ) {
		if ( NULL != widget->getUIStyle() ) {
			widget->getUIStyle()->removeFromSubscribedWidgets( mWidget );
		}
	}

	mRelatedWidgets.clear();
}

void UIStyle::applyStyleSheetProperty( const StyleSheetProperty& property,
									   std::shared_ptr<ElementDefinition> prevDefinition ) {
	const PropertyDefinition* propertyDefinition = property.getPropertyDefinition();

	// Save default value if possible and not available.
	if ( mCurrentState != UIState::StateFlagNormal ||
		 ( mCurrentState == UIState::StateFlagNormal && property.isVolatile() ) ) {
		const StyleSheetProperty* oldAttribute = getStatelessStyleSheetProperty( property.getId() );
		if ( nullptr == oldAttribute && getPreviousState() == UIState::StateFlagNormal ) {
			std::string value(
				mWidget->getPropertyString( propertyDefinition, property.getIndex() ) );
			if ( !value.empty() ) {
				setStyleSheetProperty(
					StyleSheetProperty( propertyDefinition, value, property.getIndex() ) );
			}
		}
	}

	if ( !mDisableAnimations && !mWidget->isSceneNodeLoading() && NULL != propertyDefinition &&
		 StyleSheetPropertyAnimation::animationSupported( propertyDefinition->getType() ) &&
		 hasTransition( property.getName() ) &&
		 !hasAnimation( property.getPropertyDefinition() ) ) {
		std::string currentValue =
			mWidget->getPropertyString( propertyDefinition, property.getIndex() );
		std::string startValue( currentValue );

		if ( !startValue.empty() ) {
			// Get the real start value
			auto prevProp = prevDefinition->getProperty( property.getId() );
			if ( nullptr != prevProp ) {
				StyleSheetProperty* curProperty = prevProp;
				if ( propertyDefinition->isIndexed() &&
					 property.getIndex() < curProperty->getPropertyIndexCount() ) {
					startValue = curProperty->getPropertyIndex( property.getIndex() ).getValue();
				} else {
					startValue = curProperty->getValue();
				}
			}

			TransitionDefinition transitionInfo( getTransition( property.getName() ) );

			std::vector<Action*> previousTransitions =
				mWidget->getActionsByTag( propertyDefinition->getId() );
			std::vector<Action*> removeTransitions;
			StyleSheetPropertyAnimation* prevTransition = NULL;

			if ( !previousTransitions.empty() ) {
				for ( auto& transition : previousTransitions ) {
					if ( transition->getId() == StyleSheetPropertyAnimation::ID ) {
						StyleSheetPropertyAnimation* tmpTransition =
							static_cast<StyleSheetPropertyAnimation*>( transition );
						if ( tmpTransition->getAnimationOrigin() == AnimationOrigin::Transition ) {
							if ( propertyDefinition->isIndexed() ) {
								if ( tmpTransition->getPropertyIndex() == property.getIndex() ) {
									prevTransition = tmpTransition;
									removeTransitions.push_back( prevTransition );
								}
							} else {
								prevTransition = tmpTransition;
								removeTransitions.push_back( prevTransition );
								break;
							}
						}
					}
				}
			}

			Time elapsed( Time::Zero );

			if ( NULL != prevTransition ) {
				if ( prevTransition->getEndValue() == property.getValue() ) {
					return;
				} else if ( prevTransition->getStartValue() == property.getValue() ) {
					Float currentProgress = prevTransition->getCurrentProgress();
					currentProgress = eemin( 1.f, currentProgress );
					if ( 0.f != currentProgress ) {
						elapsed = Milliseconds( ( 1.f - currentProgress ) *
												transitionInfo.getDuration().asMilliseconds() );
					} else {
						elapsed = transitionInfo.getDuration();
					}
				} else if ( startValue == prevTransition->getEndValue() ) {
					startValue = currentValue;
				}

				for ( auto& rem : removeTransitions ) {
					mWidget->removeAction( rem );
				}
			}

			StyleSheetPropertyAnimation* newTransition = StyleSheetPropertyAnimation::New(
				propertyDefinition, startValue, property.getValue(), property.getIndex(),
				transitionInfo.getDuration(), transitionInfo.getDelay(),
				transitionInfo.getTimingFunction(), AnimationOrigin::Transition );
			newTransition->setElapsed( elapsed );
			newTransition->setTag( propertyDefinition->getId() );
			mWidget->runAction( newTransition );
		} else {
			mWidget->applyProperty( property );
		}
	} else {
		mWidget->applyProperty( property );
	}
}

void UIStyle::updateAnimations() {
	bool isDifferent = false;
	CSS::AnimationsMap animations;

	if ( !mAnimationProperties.empty() ) {
		animations = AnimationDefinition::parseAnimationProperties( mAnimationProperties );
		if ( animations.size() == mAnimations.size() ) {
			for ( auto& animation : animations ) {
				auto animIt = mAnimations.find( animation.second.getName() );
				if ( animIt == mAnimations.end() || animIt->second != animation.second ) {
					isDifferent = true;
					break;
				}
			}
		} else {
			isDifferent = true;
		}
	} else if ( !mAnimations.empty() && mAnimationProperties.empty() ) {
		isDifferent = true;
	}

	if ( isDifferent ) {
		mAnimations.clear();

		removeAllAnimations();

		startAnimations( animations );
	} else if ( !mAnimationProperties.empty() ) {
		updateAnimationsPlayState();
	}
}

void UIStyle::updateAnimationsPlayState() {
	if ( mAnimations.empty() )
		return;
	std::vector<Action*> actions = mWidget->getActions();
	for ( auto& action : actions ) {
		if ( action->getId() == StyleSheetPropertyAnimation::ID ) {
			StyleSheetPropertyAnimation* animation =
				static_cast<StyleSheetPropertyAnimation*>( action );
			if ( animation->getAnimationOrigin() == AnimationOrigin::Animation ) {
				// Check all the active animations.
				size_t animPos = 0;
				for ( auto anim = mAnimations.begin(); anim != mAnimations.end(); anim++ ) {
					// Find the animation index by iterating over them...
					if ( anim->first == animation->getAnimation().getName() ) {
						// Once found the iteration index of the corresponding keyframe animation
						// First check if in the current animation properties is there any
						// "animation-play-state" definition.
						bool isSet = false;
						for ( auto& animProp : mAnimationProperties ) {
							if ( NULL != animProp->getPropertyDefinition() &&
								 animProp->getPropertyDefinition()->getPropertyId() ==
									 PropertyId::AnimationPlayState ) {
								// If found, get the pause/running state of the property, using the
								// index of the current animation, and set the animation.play-state.
								size_t animPropCount = animProp->getPropertyIndexCount();
								bool paused = animProp->getPropertyIndex( animPos % animPropCount )
														  .getValue() == "paused"
												  ? true
												  : false;
								animation->setPaused( paused );
								isSet = true;
								break;
							}
						}
						// If animation-play-state if set, continue with the next action.
						if ( isSet )
							break;
						// Otherwise set the animation-play-state defined on the animation.
						animation->setPaused( animation->getAnimation().isPaused() );
					}
					animPos++;
				}
			}
		}
	}
}

void UIStyle::startAnimations( const CSS::AnimationsMap& animations ) {
	UISceneNode* uiSceneNode = mWidget->getUISceneNode();

	if ( NULL == uiSceneNode )
		return;

	mAnimations = animations;

	CSS::StyleSheet& styleSheet = uiSceneNode->getStyleSheet();
	for ( auto& anim : animations ) {
		if ( styleSheet.isKeyframesDefined( anim.first ) ) {
			const AnimationDefinition& animation = anim.second;
			const KeyframesDefinition& keyframes = styleSheet.getKeyframesDefinition( anim.first );
			auto propDefMap = keyframes.getPropertyDefinitionList();

			for ( auto& propertyDef : propDefMap ) {
				const PropertyDefinition* propDef = propertyDef.second;

				if ( StyleSheetPropertyAnimation::animationSupported( propDef->getType() ) ) {
					if ( propDef->isIndexed() ) {
						StyleSheetProperty* prop = mDefinition->getProperty( propDef->getId() );
						if ( nullptr != prop ) {
							for ( size_t i = 0; i < prop->getPropertyIndexCount(); i++ ) {
								removeAnimation( propDef, i );
								StyleSheetPropertyAnimation* newAnimation =
									StyleSheetPropertyAnimation::fromAnimationKeyframes(
										animation, keyframes, propDef, mWidget, i );
								newAnimation->setFlags( animation.getId() );
								newAnimation->setTag( propDef->getId() );
								mWidget->runAction( newAnimation );
							}
						} else {
							removeAnimation( propDef, 0 );
							StyleSheetPropertyAnimation* newAnimation =
								StyleSheetPropertyAnimation::fromAnimationKeyframes(
									animation, keyframes, propDef, mWidget, 0 );
							newAnimation->setFlags( animation.getId() );
							newAnimation->setTag( propDef->getId() );
							mWidget->runAction( newAnimation );
						}
					} else {
						removeAnimation( propDef, 0 );
						StyleSheetPropertyAnimation* newAnimation =
							StyleSheetPropertyAnimation::fromAnimationKeyframes(
								animation, keyframes, propDef, mWidget, 0 );
						newAnimation->setFlags( animation.getId() );
						newAnimation->setTag( propDef->getId() );
						mWidget->runAction( newAnimation );
					}
				}
			}
		}
	}
}

void UIStyle::removeAllAnimations() {
	std::vector<Action*> actions = mWidget->getActions();
	std::vector<Action*> removeList;
	for ( auto& action : actions ) {
		if ( action->getId() == StyleSheetPropertyAnimation::ID ) {
			StyleSheetPropertyAnimation* animation =
				static_cast<StyleSheetPropertyAnimation*>( action );
			if ( animation->getAnimationOrigin() == AnimationOrigin::Animation ) {
				removeList.push_back( action );
			}
		}
	}
	if ( !removeList.empty() ) {
		for ( auto& action : removeList ) {
			static_cast<StyleSheetPropertyAnimation*>( action )->notifyClose();
		}
		mWidget->removeActions( removeList );
	}
}

void UIStyle::removeAnimation( const PropertyDefinition* propertyDefinition,
							   const Uint32& propertyIndex ) {
	std::vector<Action*> previousTransitions =
		mWidget->getActionsByTag( propertyDefinition->getId() );
	std::vector<Action*> removeTransitions;
	StyleSheetPropertyAnimation* prevTransition = NULL;

	if ( !previousTransitions.empty() ) {
		for ( auto& transition : previousTransitions ) {
			if ( transition->getId() == StyleSheetPropertyAnimation::ID ) {
				StyleSheetPropertyAnimation* tmpTransition =
					static_cast<StyleSheetPropertyAnimation*>( transition );
				if ( propertyDefinition->isIndexed() ) {
					if ( tmpTransition->getPropertyIndex() == propertyIndex ) {
						prevTransition = tmpTransition;
						removeTransitions.push_back( prevTransition );
					}
				} else {
					prevTransition = tmpTransition;
					removeTransitions.push_back( prevTransition );
					break;
				}
			}
		}

		mWidget->removeActions( removeTransitions );
	}
}

StyleSheetProperty* UIStyle::getLocalProperty( Uint32 propId ) {
	StyleSheetProperty* property = mDefinition->getProperty( propId );
	if ( nullptr == property )
		return mElementStyle->getPropertyById( propId );
	return property;
}

}} // namespace EE::UI

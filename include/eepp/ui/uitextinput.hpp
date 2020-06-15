#ifndef EE_UICUITEXTINPUT_H
#define EE_UICUITEXTINPUT_H

#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/keyboardshortcut.hpp>
#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/window/inputtextbuffer.hpp>

using namespace EE::UI::Doc;

namespace EE { namespace UI {

class EE_API UITextInput : public UITextView, public TextDocument::Client {
  public:
	static UITextInput* New();

	static UITextInput* NewWithTag( const std::string& tag );

	UITextInput();

	explicit UITextInput( const std::string& tag );

	virtual ~UITextInput();

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	virtual void scheduledUpdate( const Time& time );

	virtual void draw();

	virtual void setTheme( UITheme* Theme );

	UITextInput* setAllowEditing( const bool& allow );

	const bool& isEditingAllowed() const;

	virtual const String& getText();

	virtual UITextView* setText( const String& text );

	virtual void shrinkText( const Uint32& MaxWidth );

	UITextInput* setMaxLength( const Uint32& maxLength );

	const Uint32& getMaxLength();

	virtual bool applyProperty( const StyleSheetProperty& attribute );

	virtual std::string getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex = 0 );

	virtual UIWidget* setPadding( const Rectf& padding );

	const String& getHint() const;

	UITextInput* setHint( const String& hint );

	const Color& getHintColor() const;

	UITextInput* setHintColor( const Color& hintColor );

	const Color& getHintShadowColor() const;

	UITextInput* setHintShadowColor( const Color& shadowColor );

	Font* getHintFont();

	UITextInput* setHintFont( Font* font );

	Uint32 getHintCharacterSize() const;

	UITextView* setHintCharacterSize( const Uint32& characterSize );

	const Uint32& getHintFontStyle() const;

	UITextView* setHintFontStyle( const Uint32& fontStyle );

	const Float& getHintOutlineThickness() const;

	UITextView* setHintOutlineThickness( const Float& outlineThickness );

	const Color& getHintOutlineColor() const;

	UITextView* setHintOutlineColor( const Color& outlineColor );

	/** Block all the inserts, allow only numeric characters. */
	void setAllowOnlyNumbers( const bool& onlyNumbers, const bool& allowFloat = false );

	/** @return If is only allowing numbers */
	bool onlyNumbersAllowed();

	/** @return If is only allowing numbers, it allow floating point numbers? */
	bool floatingPointAllowed();

	TextDocument& getDocument();

	KeyBindings& getKeyBindings();

  protected:
	TextDocument mDoc;
	Float mWaitCursorTime;
	Vector2f mCurPos;
	Text* mHintCache;
	FontStyleConfig mHintStyleConfig;
	int mCursorPos;
	bool mAllowEditing;
	bool mShowingWait;
	bool mOnlyNumbers;
	bool mAllowFloat;
	Uint32 mMaxLength{0};
	KeyBindings mKeyBindings;

	void resetWaitCursor();

	virtual void alignFix();

	virtual void onAutoSize();

	virtual void onSizeChange();

	void autoPadding();

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onFocus();

	virtual Uint32 onFocusLoss();

	virtual Uint32 onPressEnter();

	virtual void onFontChanged();

	void onThemeLoaded();

	virtual void onCursorPosChange();

	void drawWaitingCursor();

	void updateWaitingCursor( const Time& time );

	virtual void updateText();

	virtual void selCurInit( const Int32& init );

	virtual void selCurEnd( const Int32& end );

	virtual Int32 selCurInit();

	virtual Int32 selCurEnd();

	virtual void onDocumentTextChanged();

	virtual void onDocumentCursorChange( const TextPosition& );

	virtual void onDocumentSelectionChange( const TextRange& );

	virtual void onDocumentLineCountChange( const size_t& lastCount, const size_t& newCount );

	virtual void onDocumentLineChanged( const Int64& lineIndex );

	void registerKeybindings();

	void registerCommands();

	void copy();

	void cut();

	void paste();

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onTextInput( const TextInputEvent& event );
};

}} // namespace EE::UI

#endif

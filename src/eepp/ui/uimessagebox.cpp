#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

UIMessageBox* UIMessageBox::New( const Type& type, const String& message,
								 const Uint32& windowFlags ) {
	return eeNew( UIMessageBox, ( type, message, windowFlags ) );
}

UIMessageBox::UIMessageBox( const Type& type, const String& message, const Uint32& windowFlags ) :
	UIWindow(), mMsgBoxType( type ), mTextInput( NULL ), mCloseShortcut( KEY_UNKNOWN ) {
	mStyleConfig.WinFlags = windowFlags;

	updateWinFlags();

	mLayoutCont = UILinearLayout::New();
	mLayoutCont->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setParent( mContainer );

	UILinearLayout* vlay = UILinearLayout::NewVertical();
	vlay->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutMargin( Rectf( 8, 8, 8, 8 ) )
		->setClipType( ClipType::None )
		->setParent( mLayoutCont );

	mTextBox = UITextView::New();
	mTextBox->setText( message )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setParent( vlay );

	if ( mMsgBoxType == INPUT ) {
		mTextInput = UITextInput::New();
		mTextInput->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent )
			->setLayoutMargin( Rectf( 0, 4, 0, 4 ) )
			->setParent( vlay )
			->addEventListener( Event::OnPressEnter, [&]( const Event* ) {
				sendCommonEvent( Event::MsgBoxConfirmClick );
			} );
	}

	UILinearLayout* hlay = UILinearLayout::NewHorizontal();
	hlay->setLayoutMargin( Rectf( 0, 8, 0, 0 ) )
		->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent )
		->setLayoutGravity( UI_HALIGN_RIGHT | UI_VALIGN_CENTER )
		->setClipType( ClipType::None )
		->setParent( vlay );

	mButtonOK = UIPushButton::New();
	mButtonOK->setSize( 90, 0 )->setParent( hlay );

	mButtonCancel = UIPushButton::New();
	mButtonCancel->setLayoutMargin( Rectf( 8, 0, 0, 0 ) )->setSize( 90, 0 )->setParent( hlay );

	switch ( mMsgBoxType ) {
		case UIMessageBox::INPUT:
		case UIMessageBox::OK_CANCEL: {
			mButtonOK->setText( getTranslatorString( "@string/msg_box_ok", "Ok" ) );
			mButtonCancel->setText( getTranslatorString( "@string/msg_box_cancel", "Cancel" ) );
			break;
		}
		case UIMessageBox::YES_NO: {
			mButtonOK->setText( getTranslatorString( "@string/msg_box_yes", "Yes" ) );
			mButtonCancel->setText( getTranslatorString( "@string/msg_box_no", "No" ) );
			break;
		}
		case UIMessageBox::RETRY_CANCEL: {
			mButtonOK->setText( getTranslatorString( "@string/msg_box_retry", "Retry" ) );
			mButtonCancel->setText( getTranslatorString( "@string/msg_box_cancel", "Cancel" ) );
			break;
		}
		case UIMessageBox::OK: {
			mButtonOK->setText( getTranslatorString( "@string/msg_box_ok", "Ok" ) );
			mButtonCancel->setVisible( false );
			mButtonCancel->setEnabled( false );
			break;
		}
	}

	reloadStyle( true, true );

	applyDefaultTheme();

	setMinWindowSize( mLayoutCont->getSize() );

	center();
}

UIMessageBox::~UIMessageBox() {}

void UIMessageBox::setTheme( UITheme* theme ) {
	UIWindow::setTheme( theme );

	mTextBox->setTheme( theme );
	mButtonOK->setTheme( theme );
	mButtonCancel->setTheme( theme );

	if ( getTranslatorString( "@string/msg_box_retry", "Retry" ) != mButtonOK->getText() ) {
		Drawable* okIcon = getUISceneNode()->findIconDrawable( "ok", PixelDensity::dpToPxI( 16 ) );
		Drawable* cancelIcon =
			getUISceneNode()->findIconDrawable( "cancel", PixelDensity::dpToPxI( 16 ) );

		if ( NULL != okIcon ) {
			mButtonOK->setIcon( okIcon );
		}

		if ( NULL != cancelIcon ) {
			mButtonCancel->setIcon( cancelIcon );
		}
	}

	onThemeLoaded();
}

Uint32 UIMessageBox::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseClick: {
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( Msg->getSender() == mButtonOK ) {
					sendCommonEvent( Event::MsgBoxConfirmClick );
					closeWindow();
				} else if ( Msg->getSender() == mButtonCancel ) {
					sendCommonEvent( Event::MsgBoxCancelClick );
					closeWindow();
				}
			}

			break;
		}
	}

	return UIWindow::onMessage( Msg );
}

UITextView* UIMessageBox::getTextBox() const {
	return mTextBox;
}

UIPushButton* UIMessageBox::getButtonOK() const {
	return mButtonOK;
}

UIPushButton* UIMessageBox::getButtonCancel() const {
	return mButtonCancel;
}

Uint32 UIMessageBox::onKeyUp( const KeyEvent& event ) {
	if ( mCloseShortcut && event.getKeyCode() == mCloseShortcut &&
		 ( mCloseShortcut.mod == 0 || ( event.getMod() & mCloseShortcut.mod ) ) ) {
		sendCommonEvent( Event::MsgBoxCancelClick );
		closeWindow();
	}

	return 1;
}

bool UIMessageBox::show() {
	bool b = UIWindow::show();
	if ( NULL != mTextInput ) {
		mTextInput->setFocus();
	} else {
		mButtonOK->setFocus();
	}
	return b;
}

const KeyBindings::Shortcut& UIMessageBox::getCloseShortcut() const {
	return mCloseShortcut;
}

void UIMessageBox::setCloseShortcut( const KeyBindings::Shortcut& closeWithKey ) {
	mCloseShortcut = closeWithKey;
}

UITextInput* UIMessageBox::getTextInput() const {
	return mTextInput;
}

void UIMessageBox::onWindowReady() {
	forcedApplyStyle();

	Sizef size( mLayoutCont->getSize() );
	Sizef minWinTitle( getMinWindowTitleSizeRequired() );
	if ( size.x < minWinTitle.x ) {
		size.x = minWinTitle.x;
		mLayoutCont->setLayoutWidthPolicy( SizePolicy::MatchParent );
		mLayoutCont->getFirstChild()->asType<UIWidget>()->setLayoutWidthPolicy(
			SizePolicy::MatchParent );
		mTextBox->setLayoutWidthPolicy( SizePolicy::MatchParent );
	}
	setMinWindowSize( size );
	center();

	if ( mShowWhenReady ) {
		mShowWhenReady = false;
		show();
	}

	sendCommonEvent( Event::OnWindowReady );
}

}} // namespace EE::UI

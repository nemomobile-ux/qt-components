/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "sdeclarativeincallindicator.h"

#include <AknIndicatorContainer.h>
#include <avkon.hrh>
#include <coecntrl.h>
#include <AknUtils.h>
#include <aknappui.h>
#include <layoutmetadata.cdl.h>
#include <aknlayoutscalable_apps.cdl.h>

// #define Q_DEBUG_INCALL

#ifdef Q_DEBUG_INCALL
#include <QDebug>
#endif // Q_DEBUG_INCALL

CSDeclarativeIncallIndicator* CSDeclarativeIncallIndicator::NewL()
    {
    CSDeclarativeIncallIndicator* self = new ( ELeave ) CSDeclarativeIncallIndicator();
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }

CSDeclarativeIncallIndicator::CSDeclarativeIncallIndicator()
    {
    }

void CSDeclarativeIncallIndicator::ConstructL()
    {
    }

CSDeclarativeIncallIndicator::~CSDeclarativeIncallIndicator()
    {
    CCoeEnv* coeEnv = CCoeEnv::Static();
    if ( coeEnv )
        {
        coeEnv->RemoveMessageMonitorObserver( *this );
        }
    }

void CSDeclarativeIncallIndicator::CreateIncallControlL()
    {
    if (!iControl)
        {
        // Creates CAknIndicatorContainer which internally creates the CIncallStatusBubble
        CAknIndicatorContainer* control = new ( ELeave ) CAknIndicatorContainer();
        CleanupStack::PushL( control );
        control->ConstructL();
        control->ActivateL();
        CleanupStack::Pop( control );
        iControl = control;
        }

#ifdef Q_DEBUG_INCALL
    if (!iControl)
        {
        qDebug() << "CSDeclarativeIncallIndicator::CreateIncallControlL() Cannot create CAknIndicatorContainer!";
        }
#endif

    if (!iIncallBubble)
        {
        // When setting the visible flag on for the first time, the CIncallStatusBubble
        // instance in created. Internally it creates new window group and window.
        CCoeEnv* coeEnv = CCoeEnv::Static();
        RWsSession& wsSession = coeEnv->WsSession();

        CArrayFixFlat<TInt>* oldWindowGroups = new ( ELeave ) CArrayFixFlat<TInt>( 2 );
        CleanupStack::PushL( oldWindowGroups );
        CArrayFixFlat<TInt>* newWindowGroups = new ( ELeave ) CArrayFixFlat<TInt>( 2 );
        CleanupStack::PushL( newWindowGroups );

        User::LeaveIfError( wsSession.WindowGroupList( oldWindowGroups ) );
        iControl->SetIncallBubbleFlags( EAknStatusBubbleVisible );
        User::LeaveIfError( wsSession.WindowGroupList( newWindowGroups ) );

#ifdef Q_DEBUG_INCALL
        if ( newWindowGroups->Count() - 1 != oldWindowGroups->Count() )
            {
            qDebug() << "CSDeclarativeIncallIndicator::CreateIncallControlL() mismatch old" << (int)oldWindowGroups->Count() << " new " << (int)newWindowGroups->Count();
            }
#endif

        // find the one that does not match - that is the new window group
        for (TInt newIndex = 0; newIndex < newWindowGroups->Count(); newIndex++)
            {
            const TInt id = newWindowGroups->At( newIndex );
            TBool found = EFalse;
            for (TInt oldIndex = 0; oldIndex < oldWindowGroups->Count() && !found; oldIndex++)
                {
                if (oldWindowGroups->At( oldIndex ) == id)
                    {
                    found = ETrue;
                    }
                }

            if (!found)
                {
                // found a window group which was created within the SetIncallBubbleFlags function
                // call
#ifdef Q_DEBUG_INCALL
                qDebug() << "CSDeclarativeIncallIndicator::CreateIncallControlL() found new wg id " << (int)id;
#endif
                RWindowGroup* wg = reinterpret_cast<RWindowGroup*>( wsSession.GetWindowGroupHandle( id ) );
                if ( wg && wg->Child() )
                    {
                    TUint32 child = wg->Child();
                    iIncallBubble = reinterpret_cast<CCoeControl*>( child ) ;
#ifdef Q_DEBUG_INCALL
                    qDebug() << "CSDeclarativeIncallIndicator::CreateIncallControlL() found iIncallBubble " << (int)iIncallBubble;
#endif
                    break;
                    }
                }
            }
        CleanupStack::PopAndDestroy( 2 ); // newWindowGroups, oldWindowGroups

        iIsForeground = static_cast<CAknAppUi*>( coeEnv->AppUi() )->IsForeground();
#ifdef Q_DEBUG_INCALL
        qDebug() << "CSDeclarativeIncallIndicator::CreateIncallControlL() iIsForeground " << (int)iIsForeground;
#endif
        coeEnv->AddMessageMonitorObserverL( *this );
        }

#ifdef Q_DEBUG_INCALL
    if (!iIncallBubble)
        {
        qDebug() << "CSDeclarativeIncallIndicator::CreateIncallControlL() Cannot find iIncallBubble!";
        }
#endif
    }

void CSDeclarativeIncallIndicator::MonitorWsMessage( const TWsEvent& aEvent )
    {
    switch ( aEvent.Type() )
        {
        case KAknFullOrPartialForegroundGained:
            iIsForeground = ETrue;
            UpdateIncallBubbleVisibility();
            break;

        case KAknFullOrPartialForegroundLost:
            iIsForeground = EFalse;
            UpdateIncallBubbleVisibility();
            break;

        case EEventScreenDeviceChanged: // flow through
        case KEikDynamicLayoutVariantSwitch: // flow through
        case EEventWindowVisibilityChanged:
            IncallBubbleSizeChanged();
            break;

        default:
            break;
        }
    }

void CSDeclarativeIncallIndicator::SetFlags( TInt aFlags )
    {
#ifdef Q_DEBUG_INCALL
    qDebug() << "CSDeclarativeIncallIndicator::SetFlags() flags " << (int)aFlags;
#endif
    if ( aFlags != iFlags )
        {
        iFlags = aFlags;

        if ( !iIncallBubble && (aFlags&EAknStatusBubbleVisible) )
            {
            TRAP_IGNORE( CreateIncallControlL() );
            }

        if ( iIncallBubble )
            {
            iControl->SetIncallBubbleFlags( aFlags );
            if ( !(aFlags&EAknStatusBubbleVisible) )
                {
#ifdef Q_DEBUG_INCALL
                qDebug() << "CSDeclarativeIncallIndicator::SetFlags() nulling iIncallBubble";
#endif
                iIncallBubble = NULL;
                CCoeEnv* coeEnv = CCoeEnv::Static();
                if ( coeEnv )
                    {
                    coeEnv->RemoveMessageMonitorObserver( *this );
                    }
                }
            else
                {
                UpdateIncallBubbleVisibility();
                }
            }
        }
    }

void CSDeclarativeIncallIndicator::UpdateIncallBubbleVisibility()
    {
    if ( iIncallBubble )
        {
        if ( (iFlags&EAknStatusBubbleVisible) && iIsForeground)
            {
#ifdef Q_DEBUG_INCALL
             qDebug() << "CSDeclarativeIncallIndicator::UpdateIncallBubbleVisibility() visible with new size";
#endif
            IncallBubbleSizeChanged();
            iIncallBubble->MakeVisible( ETrue );
            }
        else
            {
#ifdef Q_DEBUG_INCALL
             qDebug() << "CSDeclarativeIncallIndicator::UpdateIncallBubbleVisibility() invisible";
#endif
            iIncallBubble->MakeVisible( EFalse );
            }
        }
    }

void CSDeclarativeIncallIndicator::IncallBubbleSizeChanged()
    {
    if ( iIncallBubble )
        {
        TAknLayoutRect layoutRect;
        TAknWindowLineLayout layout;
        TRect screenRect( iAvkonAppUi->ApplicationRect() );

        if ( Layout_Meta_Data::IsLandscapeOrientation() )
            {
            layout =
                AknLayoutScalable_Apps::popup_call_status_window( 6 ).LayoutLine();

            if ( AknLayoutUtils::LayoutMirrored() )
                {
                TInt tmp = layout.il;
                layout.il = layout.ir;
                layout.ir = tmp;
                }

            TAknWindowComponentLayout topLayout =
                AknLayoutScalable_Apps::area_top_pane( 2 );

            layoutRect.LayoutRect( screenRect, topLayout );
            layoutRect.LayoutRect( layoutRect.Rect(), layout );
            }
        else
            {
            layout =
                AknLayoutScalable_Apps::popup_call_status_window( 0 ).LayoutLine();
            layoutRect.LayoutRect( screenRect, layout );
            }

        // SizeChanged of incall indicator is heavyweight, so set size only if
        // necessary.
        TRect rect( layoutRect.Rect() );
        if ( rect != TRect( iIncallBubble->Position(), iIncallBubble->Size() ) )
            {
            iIncallBubble->SetRect( rect );
            }
        }
    }



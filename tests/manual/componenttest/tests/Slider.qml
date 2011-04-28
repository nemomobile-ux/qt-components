/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project on Qt Labs.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions contained
** in the Technology Preview License Agreement accompanying this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
****************************************************************************/

import Qt 4.7
import com.nokia.symbian 1.0

Item {
    id: root

    function isPortrait() {
        return screen.height > screen.width
    }
    property variant focusItem: slider1.activeFocus ? slider1
                              : slider2.activeFocus ? slider2
                              : slider3.activeFocus ? slider3
                              : null

    Rectangle {
        border {color: "steelblue"; width: 5}
        color: "#00000000"; radius: 5; opacity: 0.80
        property int margins: focusItem && focusItem.parent ? focusItem.parent.anchors.margins : 0
        x: focusItem ? focusItem.x + margins : 0
        y: focusItem ? focusItem.y + margins : 0
        height: focusItem ? focusItem.height : 0
        width: focusItem ? focusItem.width : 0
        visible: focusItem ? focusItem.activeFocus : false
    }

    Grid {
        id: sliderGrid

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: slider3.top
            margins: 20
        }
        columns: isPortrait() ? 2 : 4;
        spacing: 10

        Text {
            text: "Horizontal (" + slider1.value + ")"
            width: 150
            color: "white"
        }

        Slider {
            id: slider1
            objectName: "slider1"
            width: parent.width / 3
            height: 50
            focus: true
            maximumValue: 125
            minimumValue: -125
            value: 150
            stepSize: 5
            valueIndicatorVisible: valueIndicatorToggle.checked
            inverted: inversionToggle.checked

            KeyNavigation.up: slider3
            KeyNavigation.down: slider2
            KeyNavigation.left: slider3
            KeyNavigation.right: slider2
        }

        CheckBox {
            id: valueIndicatorToggle
            text: "Value Ind."
            checked: true
        }

        CheckBox {
            id: inversionToggle
            text: "Inverted"
        }

        Text {
            text: "Vertical (" + slider2.value + ")"
            width: 150
            color: "white"
        }

        Slider {
            id: slider2
            objectName: "slider2"
            width: 50
            height: parent.height / 2
            orientation: Qt.Vertical
            maximumValue: 5
            stepSize: 1
            value: 5
            valueIndicatorVisible: valueIndicatorToggle2.checked
            inverted: inversionToggle2.checked

            KeyNavigation.up: slider1
            KeyNavigation.down: slider3
            KeyNavigation.left: slider1
            KeyNavigation.right: slider3
        }

        CheckBox {
            id: valueIndicatorToggle2
            text: "Value Ind."
            checked: true
        }

        CheckBox {
            id: inversionToggle2
            text: "Inverted"
        }
    }

    Slider {
        id: slider3
        objectName: "slider3"
        valueIndicatorVisible: true
        valueIndicatorText: "Custom: " + (Math.round(value * 1000) / 1000)
        stepSize: 0.0001
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right; margins: 20 }

        KeyNavigation.up: slider2
        KeyNavigation.down: slider1
        KeyNavigation.left: slider2
        KeyNavigation.right: slider1
    }
}

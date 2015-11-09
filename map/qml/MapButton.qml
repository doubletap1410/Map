import QtQuick 2.4
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0

Item {
    property string img: "qrc:///Images/Move_map_white.svg";
    property color normalColor: "#A0A0FF"
    property color checkedColor: "#FF6060"
    property bool checkable: true
    property bool checked: false
    property ExclusiveGroup exclusiveGroup: null
    id: button

    width: 56
    height: width
    state: "NORMAL"

    function checkState(idx, index, flag)
    {
        if (idx === index)
        {
            if (flag !== checked)
                checked = !checked;
        }
    }

    function sendToggle() {
        toggled(checked);
    }

    onCheckedChanged: {
        state = checked ? "CHECKED" : "NORMAL";
        sendToggle();
    }

    onExclusiveGroupChanged: {
        if (exclusiveGroup)
            exclusiveGroup.bindCheckable(button)
    }

    signal toggled(bool flag)

    Rectangle {
        id: background
        anchors.fill: parent
        color: button.normalColor
        radius: width * 0.5

        smooth: true
        visible: false
    }

    MouseArea {
        property bool isPressed: false
        anchors.fill: background
        hoverEnabled: true
        onPressed: {
            button.state = "PRESSED"
            isPressed = true
        }
        onEntered: {
            button.state = isPressed ? "PRESSED" : "HOVERED"
        }
        onExited: {
            button.state = button.checked ? "CHECKED" : "NORMAL"
        }
        onReleased: {
            isPressed = false
            if (containsMouse) {
                button.state = "HOVERED"
                checked = button.checkable ? !button.checked : false
                button.sendToggle();
            }
        }
    }

    DropShadow {
        id: shadow;
        anchors.fill: background
        horizontalOffset: -1
        verticalOffset: 4
        radius: 4.0
        samples: 8
        color: "#80000000"
        transparentBorder: true
        source: background
    }

    Image {
        property double trY: 0
        id: img
        source: button.img
        width: 24
        height: width
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        transform: Translate { y: img.trY; }
    }

    states: [
        State {
            name: "NORMAL"
        },
        State {
            name: "HOVERED"
            PropertyChanges {
                target: background;
                color: Qt.lighter(button.checked ? button.checkedColor : button.normalColor, 1.1);
            }
        },
        State {
            name: "PRESSED"
            PropertyChanges {
                target: shadow;
                horizontalOffset: 0;
                verticalOffset: 1;
                radius: 1.0
            }
            PropertyChanges {
                target: background;
                color: Qt.darker(button.checked ? button.checkedColor : button.normalColor, 1.1);
            }
            PropertyChanges {
                target: img;
                trY: -0.5;
            }

        },
        State {
            name: "CHECKED"
            PropertyChanges {
                target: background;
                color: button.checkedColor;
            }
        }
    ]
}

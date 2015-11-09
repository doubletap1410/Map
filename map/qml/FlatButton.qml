import QtQuick 2.4
import QtQuick.Controls 1.3

Item {
    property string img: "qrc:///Images/Menu_white.svg"
    property color normalColor: "#00FFFFFF"
    property color hoverColor: "#A0FFFFFF"
    property bool checked: false
    property ExclusiveGroup exclusiveGroup: null
    property double defaultAngle: 0
    property double checkedAngle: defaultAngle + 90

    signal clicked(bool flag)

    id: button
    width: 48
    height: width
    state: "NORMAL"

    onCheckedChanged: {
        state = checked ? "CHECKED" : "NORMAL";
        clicked(checked)
    }

    onExclusiveGroupChanged: {
        if (exclusiveGroup)
            exclusiveGroup.bindCheckable(button)
    }

    Rectangle {
        id: background

        anchors.fill: button
        color: normalColor
        radius: 5
    }

    Image {
        id: img

        source: button.img
        width: 24
        height: width
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter

        rotation: defaultAngle
    }

    MouseArea {
        anchors.fill: background
        onPressed: {
            button.state = "PRESSED"
        }
        onReleased: {
            checked = !button.checked            
        }
    }

    states: [
        State {
            name: "NORMAL"
//            PropertyChanges {
//                target: img
//                rotation: defaultAngle
//            }
        },
        State {
            name: "PRESSED"
            PropertyChanges {
                target: background;
                color: hoverColor;
            }
//            PropertyChanges {
//                target: img;
//                rotation: button.checked ? checkedAngle : defaultAngle;
//            }
        },
        State {
            name: "CHECKED"
            PropertyChanges {
                target: background;
                color: hoverColor;
            }
//            PropertyChanges {
//                target: img;
//                rotation: checkedAngle;
//            }
        }
    ]

//    transitions: [
//        Transition {
//            from: "*"
//            to: "CHECKED"
//            RotationAnimation { target: img; duration: 100; direction: RotationAnimation.Clockwise; }
//        },
//        Transition {
//            from: "*"
//            to: "NORMAL"
//            RotationAnimation { target: img; duration: 300; direction: RotationAnimation.Clockwise; }
//        }
//    ]
}

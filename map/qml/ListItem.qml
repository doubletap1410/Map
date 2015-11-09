import QtQuick 2.4

Item {
    id: contextButtonItem
    property string button_text;
    property bool clicked;
    property int idx;
    property bool enable: true;
    property alias font_size: vButton.font.pixelSize

    signal buttonClicked;

    height: 40
    width: parent.width - 5

    function viewButtonHovered() {
        viewButton.state = "hovered";
    }

    function viewButtonExited() {
        if (clicked == false) {
            viewButton.state = "";
        } else {
            viewButton.state = "clicked"
        }
    }

    Rectangle {
        id: viewButton;
        height: contextButtonItem.height
        width: parent.width

        Text {
            id: vButton
            text: qsTr(button_text)
            width: parent.width
            anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 10 }
            font { pixelSize: 14 }
        }
        MouseArea {
            hoverEnabled: enable
            anchors.fill: parent
            enabled: enable
            onClicked: buttonClicked();
            onEntered: viewButtonHovered();
            onExited: viewButtonExited();
        }
        states: [
            State {
                name: "clicked";
                PropertyChanges { target: vButton; color: "#286E1E"; }
            },
            State {
                name: "hovered";
                PropertyChanges { target: vButton; color: "#CC2222"/*"#BEA1C6"*/; }
                PropertyChanges { target: viewButton; color: "#FFFF99"; }
            },
            State {
                name: "normal";
                PropertyChanges { target: vButton; color: "#232323"; }
            }
        ]
    }

    Rectangle {
        id: separator
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: -1
        }
        height: 1
        color: "#1a000000"
    }
}

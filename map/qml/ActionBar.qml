import QtQuick 2.4
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0

Item {
    id: bar
    width: parent.width + 2
    height: 48

    property alias color: background.color
    property alias text: title.text

    signal settingsClicked(bool flag)
    signal menuClicked(bool flag)

    signal settingsRejected()
    signal menuRejected()

    onSettingsRejected: {
        leftMenu.checked = false
    }

    onMenuRejected: {
        rightMenu.checked = false
    }

    MouseArea {
        id: eventEater
        anchors.fill: parent
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: "white"

        smooth: true
        visible: false
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

    FlatButton {
        id: leftMenu

        anchors.verticalCenter: bar.verticalCenter
        anchors.left: bar.left
        anchors.leftMargin: 6
        width: 40

        img: "qrc:///Images/Menu_white.svg"
        onClicked: {
            bar.settingsClicked(flag)
        }
    }

    FlatButton {
        id: rightMenu

        anchors.verticalCenter: bar.verticalCenter
        anchors.right: bar.right
        anchors.rightMargin: 6
        width: 40

        defaultAngle: 90
        img: "qrc:///Images/More_white.svg"

        onClicked: {
            bar.menuClicked(flag)
        }
    }

    Text {
        id: title
        x: 72
        anchors.verticalCenter: parent.verticalCenter
        font.family: "Roboto"
        font.bold: Font.DemiBold
        font.pointSize: 16
        color: "white"
    }
}

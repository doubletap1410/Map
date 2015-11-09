import QtQuick 2.4
import QtQuick.Controls 1.3

import com.sapsan.mapsettings 1.0

Item {
    id: page
    property string name: "page"
    property MapSettings settings

    signal closed()

    Rectangle {
        id: backgound
        anchors.fill: parent

        color: "#CCCCCC"

    }

    Rectangle {
        id: button
        width: 64
        height: 64

        color: 'red'

        anchors.top: parent.top
        anchors.left: parent.left

        MouseArea {
            anchors.fill: parent
            onClicked: {
                closed()
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: "Other"
    }
}

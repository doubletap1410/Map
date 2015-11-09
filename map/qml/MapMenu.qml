import QtQuick 2.4
import QtGraphicalEffects 1.0

Rectangle {
    id: contextMenuItem

    signal menuSelected(int index, string form)

    property string ok: "asd"
    property alias itemModel: elements.model

    property bool isOpen: false
    property int itemHeight: 40
    property bool enableHeader: false
    property alias header: headerTxt.text
    property int fontSize: 14

    width: 200
    height: menuHolder.height + 1
    visible: isOpen
    focus: isOpen

    Rectangle {
        id: header

        anchors {
            top: parent.top
            left: parent.left
        }

        visible: contextMenuItem.enableHeader
        height: contextMenuItem.enableHeader ? itemHeight : 0
        width: parent.width
        color: "blue"

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter

            id: headerTxt
            text: "Header"
            font {
                pixelSize: contextMenuItem.fontSize + 4
                bold: true
            }
            color : "white"
        }
    }

    Column {
        id: menuHolder
        spacing: 1
        width: parent.width
        height: contextMenuItem.itemHeight * elements.count + 1
        anchors {
            top: header.bottom
            left: parent.left
            leftMargin: 3
            topMargin: 0
        }

        Repeater {
            id: elements
            model : ["One", "Two", "Three"]
            ListItem {
                button_text: bText
                idx: index
                height: contextMenuItem.itemHeight
                font_size: contextMenuItem.fontSize
                onButtonClicked: menuSelected(idx, bPlugin);
            }
        }
    }
}

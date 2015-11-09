import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2

import com.sapsan.map 1.0
import com.sapsan.mapsettings 1.0

Window  {
    id: window

    visible: true
    width: 800
    height: 600

    property MapSettings settings
    signal init(MapFrame map)
    onInit: {
        settings = map.settings
        for (var i = 0; i < container.count; ++i)
            console.log(container.model[i].settings = settings)
    }

    StackView {
        id: stack
        anchors.fill: parent
    }

    Item {
        id: defaultPage
        visible: false
        Rectangle {
            id: background
            anchors.fill: parent
            color: 'blue'
            Column {
                anchors.top : parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 10
                spacing: 5

                Repeater {
                    id: container
                    model : [page1, page2]

                    Rectangle {
                        width: 300
                        height: 64

                        color: 'gray'
                        Text {
                            anchors.centerIn: parent
                            color: 'white'
                            text: modelData.name
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                stack.push(modelData)
                            }
                        }
                    }
                }
            }
        }
    }

    MapSettingsTiles {
        id: page1
        name: "tiles"

        visible: false
        onClosed: {
            stack.pop()
        }
    }

    MapSettingsOther {
        id: page2
        name: "others"

        visible: false
        onClosed: {
            stack.pop()
        }
    }

    Component.onCompleted: {
        stack.push(defaultPage)
    }
}

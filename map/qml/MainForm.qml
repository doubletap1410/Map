import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2

import com.sapsan.map 1.0

Window {
    id: window

    visible: true
    width: 800
    height: 600

    Rectangle {
        id: frame
        anchors.fill: parent
        focus: true

        MapFrame {
            id: map
            anchors.fill: parent;

            Keys.onPressed: {
                console.log("Focus on Map")
            }

            camera {
                latLngStr: "55.744034 37.63345"
                zoomLevel: 9
                angle: 180
            }

            Component.onCompleted: {
                map.camera.posChanged();
                map.camera.zoomChanged();
                map.camera.angleChanged();
            }

            Connections {
                target: map.camera;
                onPosChanged: {
                    panel.mapPosChanged(map.camera.latLng.x, map.camera.latLng.y);
                    panel.mapMeasureChanged(map.camera.measure, map.camera.latLng.x);
                }
                onZoomChanged: {
                    panel.mapMeasureChanged(map.camera.measure, map.camera.latLng.x);
                }
                onAngleChanged: {
                    panel.mapRotated(map.camera.angle);
                }
            }
            Connections {
                target: panel;
                onCompassClicked: {
                    map.camera.rotateTo(0, Qt.point(0, 0), 300);
                }
            }
        }

        Canvas {
            id: cross;
            anchors.centerIn: parent
            width: 10;
            height: width;
            onPaint: {
                var ctx = cross.getContext('2d');
                ctx.lineWidth = 2;
                ctx.strokeStyle = "black"

                ctx.beginPath();
                ctx.moveTo(width * 0.5,     0);
                ctx.lineTo(width * 0.5, height);
                ctx.stroke();
                ctx.beginPath();
                ctx.moveTo(0    , height * 0.5);
                ctx.lineTo(width, height * 0.5);
                ctx.stroke();
            }
        }

        MapPanel {
            id: panel;

            anchors.right: parent.right;
            anchors.bottom: parent.bottom;
        }

        Keys.onPressed: {
            console.log("Focus on Frame")
        }

        Keys.onEscapePressed: {
            Qt.quit();
        }

        ExclusiveGroup {
            id: group;
        }

        Column {
            id: controls
            spacing: 10

            anchors.top: header.bottom
            anchors.topMargin: 30
            anchors.right: map.right
            anchors.rightMargin: 10

            Repeater {
                id: repeat;
                model : map.controller.controls
                MapButton {
                    checkable: bCheckable
                    exclusiveGroup: checkable ? group : null
                    img: bImage
                    checked: index == 0 ? true : false;

                    onToggled: {
                        bFunc.call(flag)
                    }

                    Connections {
                        target: map.controller
                        onButtonStateChanged: {
                            checkState(idx, index, flag);
                        }
                    }
                }
            }
        }

        ActionBar {
            id: header;

            anchors.top: frame.top
            anchors.horizontalCenter: frame.horizontalCenter
            anchors.margins: 0
            anchors.topMargin: -1

            color: "blue"
            text: "OpenMap v0.1.PreAlpha"

            onSettingsClicked: {
                settings.isOpen = flag
            }

            Connections {
                target: settings
                onMenuSelected: {
                    header.settingsRejected()
                }
            }

            onMenuClicked: {
                menu.isOpen = flag
            }

            Connections {
                target: menu
                onMenuSelected: {
                    header.menuRejected()
                }
            }
        }

        Rectangle {
            id: fade
            anchors.fill: map
            visible: false

            signal show(bool flag)
            signal bgClicked

            color: "#66000000"
            onShow: {
                visible = flag
            }

            onBgClicked: {
                header.settingsRejected()
                header.menuRejected()
            }

            Connections {
                target: menu
                onVisibleChanged:
                    fade.show(menu.isOpen)
            }

            Connections {
                target: settings
                onVisibleChanged:
                    fade.show(settings.isOpen)
            }

            MouseArea {
                anchors.fill: parent
                onClicked: fade.bgClicked()
            }
        }

        MapMenu {
            id: menu
            isOpen: false

            anchors.top: header.bottom
            anchors.right: header.right
            anchors.rightMargin: 2

            width: 250
            fontSize: 16

            //        itemModel: ["Панель инструментов", "Построение маршрутов", "Прочее..."]
            itemModel: map.controller.panels

            onMenuSelected: {
                console.log("menu " + index + " clicked; qml: " + form)
            }
        }

        MapMenu {
            id: settings
            isOpen: false

            anchors.top: header.top
            anchors.left: header.left

            height: map.height
            width: 300
            itemHeight: 60

            enableHeader: true
            header: "Настройки"

            fontSize: 20

            //        itemModel: ["Карта", "Прочее..."]
            itemModel: map.controller.settings

            onMenuSelected: {
                console.log("settings " + index + " clicked; qml: " + form)
                comp.source = form
                comp.visible = true
            }
        }

        Loader {
            id: comp
            anchors.centerIn: parent
            source: ""
            visible: false

            MouseArea {
                anchors.fill: parent
                onClicked: parent.visible = false
            }
        }
    }
}

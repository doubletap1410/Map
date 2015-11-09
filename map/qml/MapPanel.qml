import QtQuick 2.4
import QtGraphicalEffects 1.0

import "qrc:///js/algo.js" as MathFunctions

Item {
    property double longitude: 0
    property double latitude: 0
    property double rotAngle: 0
    property double scale: 0;
    property double spacing: 5

    id: panel;

    signal mapRotated (double ang)
    signal mapPosChanged (double x, double y)
    signal mapMeasureChanged (double measure, double longitude)
    signal compassClicked()

    onMapRotated: {
        panel.rotAngle = ang;
    }
    onMapPosChanged: {
        panel.longitude = x;
        panel.latitude = y;
    }
    onMapMeasureChanged: {
        panel.scale = MathFunctions.calcScale(measure, longitude);
        panel.scale = MathFunctions.calcScale(measure, longitude);
    }

    width: compass.width + coords.width + scaleInfo.width + 2.4 * MathFunctions.PixTo1cm + 3 * spacing;
    height: 30;

    MouseArea {
        anchors.fill: parent;
        onPressed: {
            console.log("Clicked on Panel")
        }
    }

    opacity: 0.7;
    Rectangle {
        id: background;
        color: "white"
        anchors.fill: panel;
        clip: true
        smooth: true
        visible: true
    }

    DropShadow {
        anchors.fill: background
        horizontalOffset: -1
        verticalOffset: -1
        radius: 4.0
        samples: 8
        color: "#60000000"
        transparentBorder: true
        source: background
    }

    Image {
        id: compass
        height: panel.height - 2 * panel.spacing;
        width: height;
        fillMode: Image.PreserveAspectFit
        source: "qrc:///Images/Compass_red.svg";
        anchors.left: parent.left;
        anchors.verticalCenter: parent.verticalCenter;
        anchors.leftMargin: 10;
        rotation: panel.rotAngle;
        MouseArea {
            anchors.fill: parent;
            onPressed: {
                panel.compassClicked();
            }
        }
    }

    Text {
        id: style;
        height: 0;
        width: 0;

        font.family: "Times new roman";
        font.pixelSize: 12;
    }

    Rectangle {
        id: coords;
        anchors.right: parent.right;
        anchors.rightMargin: panel.spacing;
        anchors.top: panel.top;
        height: panel.height;
        width: lonVal.width + lonText.width;
        color: "transparent";
        TextMetrics {
            id: coordsMetrics
            font: style.font
            text: "X:-00'000'000"
        }
        TextMetrics {
            id: valueMetrics;
            font: style.font
            text: " W"
        }
        Text {
            id: lonVal;
            color: "black";
            width: valueMetrics.boundingRect.width;
            height: valueMetrics.boundingRect.height;
            anchors.top: coords.top;
            anchors.topMargin: Math.max((coords.height * 0.5 - height) * 0.5, 0);
            anchors.right: coords.right;
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignVCenter;
            font: style.font
            text: MathFunctions.displayLonLatDirection(panel.latitude, false);
        }
        Text {
            id: latVal;
            color: "black";
            width: valueMetrics.boundingRect.width;
            height: valueMetrics.boundingRect.height;
            anchors.bottom: parent.bottom;
            anchors.bottomMargin: Math.max((coords.height * 0.5 - height) * 0.5, 0);
            anchors.right: parent.right;
            horizontalAlignment: Text.AlignLeft;
            verticalAlignment: Text.AlignVCenter;
            font: style.font
            text: MathFunctions.displayLonLatDirection(panel.latitude, true);
        }
        Text {
            id: lonText;
            color: "black";
            width: coordsMetrics.boundingRect.width;
            height: coordsMetrics.boundingRect.height;
            anchors.top: coords.top;
            anchors.topMargin: Math.max((coords.height * 0.5 - height) * 0.5, 0);
            anchors.right: lonVal.left;
            horizontalAlignment: Text.AlignRight;
            verticalAlignment: Text.AlignVCenter;
            font: style.font
            text: MathFunctions.debugSec(panel.longitude);
        }
        Text {
            id: latText;
            color: "black";
            width: coordsMetrics.boundingRect.width;
            height: coordsMetrics.boundingRect.height;
            anchors.bottom: coords.bottom;
            anchors.bottomMargin: Math.max((coords.height * 0.5 - height) * 0.5, 0);
            anchors.right: latVal.left;
            horizontalAlignment: Text.AlignRight;
            verticalAlignment: Text.AlignVCenter;
            font: style.font
            text: MathFunctions.debugSec(panel.latitude);
        }
    }

    Rectangle {
        id: scaleInfo;
        anchors.right: coords.left;
        anchors.top: panel.top;
        height: panel.height;
        width: scaleText.width;
        color: "transparent";
        TextMetrics {
            id: scaleMetrics;
            font: style.font
            text: "M 1:000'000'000"
        }
        Text {
            id: scaleText;
            color: "black";
            width: scaleMetrics.boundingRect.width;
            height: scaleMetrics.boundingRect.height;
            //            anchors.verticalCenter: scaleInfo.verticalCenter;
            anchors.top: scaleInfo.top;
            anchors.topMargin: Math.max((scaleInfo.height * 0.5 - height) * 0.5, 0);
            anchors.right: scaleInfo.right;
            horizontalAlignment: Text.AlignRight;
            verticalAlignment: Text.AlignVCenter;
            font: style.font
            text: "M 1:" + MathFunctions.humanStringValue(panel.scale * 100);
        }
    }

    Rectangle {
        property double factor: 1.5;
        property var rulerData: MathFunctions.calcRuler(panel.scale, factor)

        id: ruler;
        anchors.top: panel.top;
        anchors.right: scaleInfo.left;
        anchors.rightMargin: panel.spacing;
        height: panel.height;
        width: rulerDraw.width;
        color: "transparent";
        TextMetrics {
            id: rulerTextMetrics;
            font: style.font
            text: ruler.rulerData[0];
        }
        Text {
            id: rulerText;
            color: "black";
            width: rulerTextMetrics.boundingRect.width;
            height: rulerTextMetrics.boundingRect.height;
            anchors.verticalCenter: ruler.verticalCenter;
            anchors.right: ruler.right;
            anchors.rightMargin: panel.spacing;
            horizontalAlignment: Text.AlignRight;
            verticalAlignment: Text.AlignVCenter;
            font: style.font
            text: ruler.rulerData[0];
        }

        Canvas {
            property double length: ruler.rulerData[1] * MathFunctions.PixTo1cm;
            id: rulerDraw;
            width: length;
            height: 8;
            anchors.right: ruler.right;
            anchors.top: rulerText.verticalCenter;
            anchors.topMargin: panel.spacing * 0.5;
            onPaint: {
                var ctx = rulerDraw.getContext('2d');
                ctx.lineWidth = 2;
                ctx.strokeStyle = "black"

                ctx.beginPath();
                ctx.moveTo(width, 0);
                ctx.lineTo(width, height);
                ctx.lineTo(0, height);
                ctx.lineTo(0, 0);
                ctx.stroke();
            }
        }
    }
}

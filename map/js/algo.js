.pragma library

var PixTo1cm = 37.70739064857;

function degToRad(x) {
    return x * Math.PI / 180;
}

function radToSec(x) {
    return x * 206264.806;
}

function formatNumberLength(num, length) {
    var r = "" + num;
    while (r.length < length) {
        r = "0" + r;
    }
    return r;
}

function debugSec(x) {
    x = radToSec(degToRad(Math.abs(x)));
    var degree = Math.floor((x / 3600));
    x -= degree * 3600;
    var min = Math.floor(x / 60);
    x -= min * 60;
    return String(degree.toString() + String.fromCharCode(176) +
                  formatNumberLength(min, 2) + String.fromCharCode(39) +
                  formatNumberLength(Math.round(x), 2) + String.fromCharCode(34));
}

// lonitude - x; latitude - y;
function displayLonLatDirection(x, latitude) {
    if (latitude) {
        return (x > 0 ? " E" : " W");
    }
    else {
        return (x > 0 ? " N" : " S");
    }
}

function calcScale(measure, longitude) {
    return Math.cos(degToRad(longitude)) * measure;
}

function humanStringValue(coord) {
    var s = Math.abs(coord.toFixed(0)).toString();
    for (var i = s.length - 3; i > 0; i -= 3)
        s = [s.slice(0, i), String.fromCharCode(39), s.slice(i)].join('')
    return s;
}

function calcMeasure(fract) {
    if (fract < 0.75)
        return 0;
    if (fract < 1.5)
//    if (fract < 1.75)
        return 1;
    if (fract < 3.5)
        return 2;
//    if (fract < 3.75)
//        return 2.5;
    if (fract < 7.5)
        return 5;
    return 0;
}

function calcRuler(measure, factor) {
    var value = 0;
    var name;
    var len = 0;

    var cm2m = measure * factor;
    for (var i = 10000000; i > 0; i /= 10) {
        var k = calcMeasure(cm2m / i);
        if (k !== 0) {
            value = k * i;
            len = (1 / measure) * value;
            if (i <= 100)
                name = "м";
            else if (i <= 100000) {
                name = "км";
                value /= 1000;
            }
            else {
                name = "тыс.км";
                value /= 1000000;
            }
            break;
        }
    }
    return [value.toFixed(0).toString() + " " + name, len];
}

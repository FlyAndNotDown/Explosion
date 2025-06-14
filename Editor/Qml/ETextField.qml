import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic

Item {
    readonly property string displayText: textField.displayText
    property string placeHolderText: ''
    property int wrapMode: TextInput.NoWrap
    property string text: textField.text
    property bool readOnly: textField.readOnly
    property bool withHintBar: false
    property color hintBarColor: ETheme.primaryColor
    property var validator: null

    signal accepted()

    id: root
    implicitWidth: textField.implicitWidth
    implicitHeight: textField.implicitHeight

    TextField {
        id: textField
        width: root.width
        height: root.height
        implicitWidth: 500
        text: root.text
        rightPadding: 10
        color: ETheme.fontColor
        placeholderText: root.placeHolderText
        placeholderTextColor: ETheme.placeHolderFontColor
        font.pixelSize: ETheme.contentFontSize
        font.family: ETheme.fontFamily
        wrapMode: root.wrapMode
        readOnly: root.readOnly
        validator: root.validator
        onAccepted: root.accepted()

        background: Rectangle {
            radius: 5
            color: ETheme.primaryBgColor
        }
    }

    Rectangle {
        id: hintBar
        anchors.bottom: textField.bottom
        width: root.width
        height: 3
        radius: 500
        visible: withHintBar
        color: hintBarColor
    }
}

/*
 * Copyright 2018 Michail Vourlakos <mvourlakos@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http: //www.gnu.org/licenses/>.
 */
import QtQuick 2.7
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.taskmanager 0.1 as TaskManager

import org.kde.appletdecoration 0.1 as AppletDecoration

Item {
    id: main

    Layout.minimumWidth: buttonsArea.width
    Layout.minimumHeight: height
    Layout.preferredHeight: Layout.minimumHeight
    Layout.preferredWidth: Layout.minimumWidth
    Layout.maximumHeight: Layout.minimumHeight
    Layout.maximumWidth: Layout.minimumWidth

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation

    // Window properties
    property bool noWindowActive: true
    property bool currentWindowMaximized: false
    property bool isActiveWindowPinned: false
    property bool isActiveWindowMaximized: false

    property string currentScheme: "kdeglobals"
    property string currentPlugin: "org.kde.breeze"

    //onCurrentSchemeChanged: initializeControlButtonsModel();

    Component.onCompleted: initializeControlButtonsModel();

    ListModel {
        id: controlButtonsModel
    }

    // To get current activity name
    TaskManager.ActivityInfo {
        id: activityInfo
    }

    // To get virtual desktop name
    TaskManager.VirtualDesktopInfo {
        id: virtualDesktopInfo
    }

    TaskManager.TasksModel {
        id: tasksModel
        sortMode: TaskManager.TasksModel.SortVirtualDesktop
        groupMode: TaskManager.TasksModel.GroupDisabled
        screenGeometry: plasmoid.screenGeometry
        activity: activityInfo.currentActivity
        virtualDesktop: virtualDesktopInfo.currentDesktop

        filterByScreen: true
        filterByVirtualDesktop: true
        filterByActivity: true

        onActiveTaskChanged: {
            activeWindowModel.sourceModel = tasksModel
            updateActiveWindowInfo()
        }
        onDataChanged: {
            updateActiveWindowInfo()
        }
    }

    // should return always one item
    PlasmaCore.SortFilterModel {
        id: activeWindowModel
        filterRole: 'IsActive'
        filterRegExp: 'true'
        sourceModel: tasksModel
        onDataChanged: {
            updateActiveWindowInfo()
        }
        onCountChanged: {
            updateActiveWindowInfo()
        }
    }

    ///Decoration Items
    AppletDecoration.Bridge {
        id: bridgeItem
        plugin: currentPlugin
    }

    AppletDecoration.Settings {
        id: settingsItem
        bridge: bridgeItem.bridge
        borderSizesIndex: 0 // Normal
    }

    ///functions
    function activeTask() {
        return activeWindowModel.get(0) || {}
    }

    function addButton(preparedArray, buttonName) {
        if (buttonName === 'close') {
            preparedArray.push({
                                   iconName: 'close',
                                   windowOperation: 'close'
                               });
        } else if (buttonName === 'maximize') {
            preparedArray.push({
                                   iconName: 'maximize',
                                   windowOperation: 'toggleMaximized'
                               });
        } else if (buttonName === 'minimize') {
            preparedArray.push({
                                   iconName: 'minimize',
                                   windowOperation: 'toggleMinimized'
                               });
        } else if ((buttonName === 'pin' || buttonName === 'alldesktops')) {
            preparedArray.push({
                                   iconName: 'alldesktops',
                                   windowOperation: 'togglePinToAllDesktops'
                               });
        }
    }

    function initializeControlButtonsModel() {
        var buttonOrder = "minimize|close";
        var preparedArray = []
        buttonOrder.split('|').forEach(function (buttonName) {
            addButton(preparedArray, buttonName);
        });

        controlButtonsModel.clear()

        for (var i = 0; i < preparedArray.length ; ++i) {
            controlButtonsModel.append(preparedArray[i])
        }
    }

    function performActiveWindowAction(windowOperation) {
        if (windowOperation === 'close') {
            toggleClose()
        } else if (windowOperation === 'toggleMaximized') {
            toggleMaximized()
        } else if (windowOperation === 'toggleMinimized') {
            toggleMinimized()
        } else if (windowOperation === 'togglePinToAllDesktops') {
            togglePinToAllDesktops()
        }
    }

    function toggleMaximized() {
        tasksModel.requestToggleMaximized(tasksModel.activeTask);
    }

    function toggleMinimized() {
        tasksModel.requestToggleMinimized(tasksModel.activeTask);
    }

    function toggleClose() {
        tasksModel.requestClose(tasksModel.activeTask);
    }

    function updateActiveWindowInfo() {
        var actTask = activeTask()
        noWindowActive = activeWindowModel.count === 0 || actTask.IsActive !== true
        currentWindowMaximized = !noWindowActive && actTask.IsMaximized === true
        isActiveWindowPinned = actTask.VirtualDesktop === -1;
    }

    ////// Visual Items

    Row {
        id: buttonsArea
        Repeater {
            model: controlButtonsModel

            AppletDecoration.Button {
                id: cButton
                anchors.topMargin: 5
                width: height
                height: main.height

                bridge: bridgeItem.bridge
                settings: settingsItem
                scheme: main.currentScheme

                type: {
                    switch(iconName) {
                    case 'menu': return 0;
                    case 'applicationmenu': return 1;
                    case 'alldesktops': return 2;
                    case 'minimize': return 3;
                    case 'maximize': return 4;
                    case 'close': return 5;
                    case 'contexthelp' : return 6;
                    case 'shade': return 7;
                    case 'keepbelow': return 8;
                    case 'keepabove':  return 9;
                    default: return 5;
                    }
                }

                onClicked: {
                    main.performActiveWindowAction(windowOperation);
                }
            }
        }
    }

}

/* Copyright (c) 2017 SKKU ESLAB, and contributors. All rights reserved.
 *
 * Contributor: Injung Hwang<sinban04@gmail.com>
 *              Dongig Sin<dongig@skku.edu>
 *              Gyeonghwan Hong<redcarrottt@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var ant_api_dir = process.env.ANT_BIN_DIR + "/api/";
var api = require(ant_api_dir + "ant");
var appApi = api.app();

var commApi = api.comm();
var sensorApi = require(ant_api_dir + "sensor-api");

var sensorData = {
  "BUTTON": -1,
  "ACC": -1,
  "MOTION": -1,
  "SOUND": -1,
  "LIGHT": -1,
  "VIBRATION": -1,
  "TEMP": -1
};

var initializeSensorData = function() {
  var sensorList = sensorApi.GetSensorlist();
  var sensorNum = sensorList.sensorNum;

  var i = 0;
  for(i = 0; i < sensorNum; i++){
    var sensorIndex = 'sensor' + (i + 1);
    sensorData[sensorList[sensorIndex]] = 0;
  }
};

var onSensorData = function() {
  sensorData.BUTTON =
    (sensorData.BUTTON >= 0) ? sensorApi.Get("BUTTON").BUTTON : -1;
  sensorData.ACC =
    (sensorData.ACC >= 0) ? sensorApi.Get("ACC").Z : -1;
  sensorData.MOTION =
    (sensorData.MOTION >= 0) ? sensorApi.Get("MOTION").MOTION : -1;
  sensorData.SOUND =
    (sensorData.SOUND >= 0) ? sensorApi.Get("SOUND").SOUND : -1;
  sensorData.LIGHT =
    (sensorData.LIGHT >= 0) ? sensorApi.Get("LIGHT").LIGHT : -1;
  sensorData.VIBRATION =
    (sensorData.VIBRATION >= 0) ? sensorApi.Get("VIBRATION").VIBRATION : -1;
  sensorData.TEMP =
    (sensorData.TEMP >= 0) ? sensorApi.Get("TEMP").TEMP : -1;

  if(sensorData.ACC === undefined) sensorData.ACC = -1;

  var str = sensorData.BUTTON
    + " " + sensorData.ACC
    + " " + sensorData.MOTION
    + " " + sensorData.SOUND
    + " " + sensorData.LIGHT
    + " " + sensorData.VIBRATION
    + " " + sensorData.TEMP;

  // Notify to companion's SensorViewer
  commApi.sendToCompanion("sensorviewer", str);
  console.log("Sent\n");
};

appApi.onLaunch(function() {
  initializeSensorData();

  // Report sensor data periodically
  setInterval(onSensorData, 500);
});

appApi.onTermination(function() {
});

appApi.appReady();

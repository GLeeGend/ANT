package com.ant.ant_manager.view.main;

import android.content.Intent;
import android.graphics.BitmapFactory;
import android.widget.Toast;

import com.ant.ant_manager.R;
import com.ant.ant_manager.view.CameraViewerActivity;
import com.ant.ant_manager.view.ImageClassifierActivity;
import com.ant.ant_manager.view.MainActivity;

/* Copyright (c) 2017 SKKU ESLAB, and contributors. All rights reserved.
 *
 * Contributor: Gyeonghwan Hong<redcarrottt@gmail.com>
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

public class ImageClassifierMainIcon extends MainIcon {
    private int mAppId;

    public ImageClassifierMainIcon(MainActivity ownerActivity, int appId) {
        super(ownerActivity, "Image Classifier", BitmapFactory.decodeResource(ownerActivity
                .getResources(), R.drawable.imageclassifier));
        this.mAppId = appId;
    }

    @Override
    public void onClick() {
        if (!this.mOwnerActivity.isTargetDeviceConnected()) {
            Toast.makeText(this.mOwnerActivity.getApplicationContext(), "Target device is not " +
                    "connected", Toast.LENGTH_SHORT).show();
            return;
        }
        Intent intent = new Intent(this.mOwnerActivity, ImageClassifierActivity.class);
        intent.putExtra(CameraViewerActivity.INTENT_KEY_APP_ID, this.mAppId);
        mOwnerActivity.startActivity(intent);
    }

    @Override
    public void onLongClick() {
        // System app -> cannot terminate
        Toast.makeText(this.mOwnerActivity, "System app cannot be terminated.", Toast
                .LENGTH_LONG).show();
    }
}
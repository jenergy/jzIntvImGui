package com.jzintvimgui;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.graphics.Rect;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Toast;
import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;
import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;

public class MainActivity extends org.libsdl.app.SDLActivity {

    /* A fancy way of getting the class name */
    private static final String TAG = MainActivity.class.getSimpleName();

    private static final String PROPERTY_FILE_NAME = "jzIntvImGui.ini";

    /* A list of assets to copy to internal directory */
    private static final String[] ASSET_NAMES = new String[]{PROPERTY_FILE_NAME};

    private static File internalSdFile = null;

    private static File externalSdFile = null;

    private static File propertiesFile = null;

    private View contentView;

    private boolean isKeyboardShowing = false;

    private boolean hardware_keyboard_available = false;

    @Override
    protected String[] getArguments() {
        return new String[]{getFilesDir().getAbsolutePath()};
    }

    private void forceCloseSoftKeyboard() {
        View view = this.getCurrentFocus();
        if (view != null) {
            InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }
    }

    private void handleSoftKeyboardHook() {
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        contentView = getWindow().getDecorView();
        contentView.getViewTreeObserver().addOnGlobalLayoutListener(
                new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        Rect r = new Rect();
                        contentView.getWindowVisibleDisplayFrame(r);
                        int screenHeight = contentView.getRootView().getHeight();

                        int keypadHeight = screenHeight - r.bottom;
                        if (keypadHeight > screenHeight * 0.15) { // 0.15 ratio is perhaps enough to determine keypad height.
                            // keyboard is opened
                            if (!isKeyboardShowing) {
                                isKeyboardShowing = true;
                                Log.d(TAG, "Soft keyboard open");
                            }
                        } else {
                            // keyboard is closed
                            if (isKeyboardShowing) {
                                isKeyboardShowing = false;
                                Log.d(TAG, "Soft keyboard closed");
                                setClosedSoftKeyboard();
                            }
                        }
                    }
                });
    }

    private Handler usbDevicesHandler = null;
    HashMap<String, Integer> usbMap = new HashMap<>();

    private void manageUsb(boolean silence) {
        StringBuilder sbDisconnected = new StringBuilder();
        StringBuilder sbConnected = new StringBuilder();
        UsbManager usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceMap = usbManager.getDeviceList();
        HashMap<String, Integer> newUsbMap = new HashMap<>();
        for (String s : deviceMap.keySet()) {
            String productName = deviceMap.get(s).getProductName();
            int oldVal = newUsbMap.get(productName) == null ? 0 : newUsbMap.get(productName).intValue();
            newUsbMap.put(productName, oldVal + 1);
        }

        for (String s : usbMap.keySet()) {
            int before = usbMap.get(s) == null ? 0 : usbMap.get(s).intValue();
            int now = newUsbMap.get(s) == null ? 0 : newUsbMap.get(s).intValue();
            if (before > now) {
                for (int i = now; i < before; i++) {
                    if (sbDisconnected.length() == 0) {
                        sbDisconnected.append("\nDisconnected:");
                    }
                    sbDisconnected.append("\n").append(s);
                }
            }
        }

        boolean old_hardware_keyboard_available = hardware_keyboard_available;
        hardware_keyboard_available = false;
        for (String s : newUsbMap.keySet()) {
            int now = newUsbMap.get(s) == null ? 0 : newUsbMap.get(s).intValue();
            int before = usbMap.get(s) == null ? 0 : usbMap.get(s).intValue();
            if (s.toLowerCase().contains("keyboard")) {
                // WEEAK
                hardware_keyboard_available = true;
            }
            if (before < now) {
                for (int i = before; i < now; i++) {
                    if (sbConnected.length() == 0) {
                        if (!silence) {
                            sbConnected.append("\nConnected:");
                        } else {
                            sbConnected.append("\nDetected:");
                        }
                    }
                    sbConnected.append("\n").append(s);
                }
            }
        }

        String msg = sbDisconnected.toString().trim() + sbConnected.toString().trim();
        if (msg.length() > 0) {
            showToast(msg);
        }
        usbMap = newUsbMap;

        if (hardware_keyboard_available && !old_hardware_keyboard_available) {
            enableTextInputForPhysicalKeyboard();
        } else if (!hardware_keyboard_available && old_hardware_keyboard_available) {
            disableTextInputForPhysicalKeyboard();
            forceFullScreen();
        }
    }

    private Handler tickHandlerLong = null;
    private Handler tickHandlerShort = null;
    private boolean tickFirst = true;

    private void handleTickHookLong() {
        if (tickHandlerLong == null) {
            tickHandlerLong = new Handler();
        }
        tickHandlerLong.postDelayed(new Runnable() {
            @Override
            public void run() {
                manageUsb(tickFirst);
                tickFirst = false;
                handleTickHookLong();
            }
        }, 1500);
    }

    private void handleTickHookShort() {
        if (tickHandlerShort == null) {
            tickHandlerShort = new Handler();
        }
        tickHandlerShort.postDelayed(new Runnable() {
            @Override
            public void run() {
                forceFullScreen();
                handleTickHookShort();
            }
        }, 500);
    }

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        handleSoftKeyboardHook();
        handleTickHookLong();
        handleTickHookShort();
        toastHandler = new Handler();
        setPaths();
        initApplicationNative();
        boolean permissionOk = isStoragePermissionGranted();
        if (permissionOk) {
            onOkPermission(false);
        } else {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
        }
    }

    private void showToastExitNoPermission() {
        final Toast toast = Toast.makeText(getContext(), "Unable to continue without permission", Toast.LENGTH_SHORT);
        toast.show();

        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                toast.cancel();
                onStop();
            }
        }, 2000);
    }

    private void handleForCopyAssets() {
        Toast.makeText(getContext(), "Copying data to sd:/jzIntvImGui, please wait for a couple of seconds...", Toast.LENGTH_LONG).show();
        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                copyAssets();
                setOkPermission();
            }
        }, 500);
    }

    private void askUserToOverwrite() {
        final boolean[] res = {false};
        DialogInterface.OnClickListener dialogClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                switch (which) {
                    case DialogInterface.BUTTON_POSITIVE:
                        // Overwrite
                        handleForCopyAssets();
                        break;

                    case DialogInterface.BUTTON_NEGATIVE:
                        // Mantain
                        setOkPermission();
                        break;
                }
            }
        };

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setMessage("A previous configuration were found. Do you want to overwrite it or mantain it?").setPositiveButton("Overwrite", dialogClickListener)
                .setNegativeButton("Mantain", dialogClickListener).show();
    }

    private void onOkPermission(boolean askForOverwrite) {
        if (!propertiesFile.exists()) {
            handleForCopyAssets();
        } else {
            if (askForOverwrite) {
                askUserToOverwrite();
            } else {
                setOkPermission();
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            Log.v(TAG, "Permission: " + permissions[0] + "was " + grantResults[0]);
            onOkPermission(true);
        } else {
            showToastExitNoPermission();
        }
    }

    public boolean isStoragePermissionGranted() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    == PackageManager.PERMISSION_GRANTED) {
                Log.v(TAG, "Permission is granted");
                return true;
            }
        } else { //permission is automatically granted on sdk<23 upon installation
            Log.v(TAG, "Permission is granted");
            return true;
        }
        return false;
    }

    private void setPaths() {
        File[] tmp = getExternalFilesDirs(null);
        for (int i = 0; i < 2; i++) {
            if (tmp.length > i) {
                String[] splitted = tmp[i].getAbsolutePath().split("/");
                StringBuilder finalPath = new StringBuilder("");
                for (int j = 0; j < splitted.length; j++) {
                    if (splitted[j].equalsIgnoreCase("ANDROID")) {
                        break;
                    }
                    if (splitted[j].trim().length() > 0) {
                        finalPath.append("/").append(splitted[j]);
                    }
                }
                if (i == 0) {
                    internalSdFile = new File(finalPath.toString());
                    propertiesFile = new File(getRootFolderForConfiguration() + PROPERTY_FILE_NAME);
                } else {
                    externalSdFile = new File(finalPath.toString());
                }
            }
        }
    }

    private void copyFileFromAsset(String assetName, FileOutputStream fos) {
        Log.v(TAG, "Copying " + assetName + " to accessible locations");
        AssetManager assetManager = this.getAssets();
        try {
            InputStream ais = assetManager.open(assetName);
            final int BUFSZ = 8192;
            byte[] buffer = new byte[BUFSZ];
            int readlen = 0;
            do {
                readlen = ais.read(buffer, 0, BUFSZ);
                if (readlen < 0) {
                    break;
                }
                fos.write(buffer, 0, readlen);
            } while (readlen > 0);
            ais.close();
        } catch (IOException e) {
            Log.e(TAG, "Could not open " + assetName + " from assets, that should not happen", e);
        }
    }

    private void copyDefaultIni() {
        try {
            propertiesFile.getParentFile().mkdirs();
            FileOutputStream fos = new FileOutputStream(propertiesFile);
            copyFileFromAsset(PROPERTY_FILE_NAME, fos);
            fos.close();
        } catch (IOException e) {
            Log.e(TAG, "Could not open " + PROPERTY_FILE_NAME + " from assets, that should not happen", e);
        }
    }

    private boolean copyOtherAssets(String path) {
        String[] list;
        try {
            list = getAssets().list(path);
            if (list.length > 0) {
                // This is a folder
                for (String file : list) {
                    if (!file.contains("resources") && !path.contains("resources")) {
                        continue;
                    }
                    if (!copyOtherAssets(path + (path.equals("") ? "" : "/") + file))
                        return false;
                    else {
                        // This is a file
                        String filename = path + "/" + file;
                        File destinationFile = new File(getRootFolderForConfiguration() + filename);
                        File parent = destinationFile.getParentFile();
                        if (!parent.exists()) {
                            parent.mkdirs();
                        }
                        if (!destinationFile.exists()) {
                            FileOutputStream fos = new FileOutputStream(destinationFile);
                            copyFileFromAsset(filename, fos);
                            fos.close();
                        }
                    }
                }
            }
        } catch (IOException e) {
            return false;
        }

        return true;
    }

    public void copyOtherAssets() {
        copyOtherAssets("");
    }

    private void copyAssets() {
        copyDefaultIni();
        copyOtherAssets();
    }

    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        super.setOrientationBis(w, h, resizable, hint);
        SDLActivity.mSingleton.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR);
    }

    @Override
    protected void onStop() {
        super.onStop();
        android.os.Process.killProcess(android.os.Process.myPid());
    }

    protected String[] getLibraries() {
        return new String[]{
                "hidapi",
                "SDL2",
                "SDL2_image",
                // "SDL2_mixer",
                // "SDL2_net",
                // "SDL2_ttf",
                "jzIntvImGui"
        };
    }

    public String getRootFolderForConfiguration() {
        return internalSdFile.getAbsolutePath() + "/jzIntvImGui/";
    }

    public String getInternalSdPath() {
        return internalSdFile.getAbsolutePath();
    }

    public String getExternalSdPath() {
        return externalSdFile == null ? "" : externalSdFile.getAbsolutePath();
    }

    public String openUrl(String url) {
        Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
        startActivity(browserIntent);
        return "";
    }

    public String forceFullScreen() {
        int flags = View.INVISIBLE;
        if (!isKeyboardShowing) {
            forceCloseSoftKeyboard();
            flags = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |   // Nasconde navigation bar
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |  // Nasconde automaticamente navigation bar
                    View.INVISIBLE;                         // Nasconde status bar

        }
        getWindow().getDecorView().setSystemUiVisibility(flags);
        return "";
    }

    public String emulationStart() {
        forceFullScreen();
        return "";
    }

    public String emulationEnd() {
        if (hardware_keyboard_available) {
            enableTextInputForPhysicalKeyboard();
        }
        return "";
    }

    // Custom toast management, if needed
    private String toastMsg;
    private Handler toastHandler;
    private Runnable toastRunnable = new Runnable() {
        @Override
        public void run() {
            Toast.makeText(getContext(), toastMsg, Toast.LENGTH_LONG).show();
        }
    };

    public String showToast(String message) {
        toastMsg = message;
        toastHandler.post(toastRunnable);
        return "";
    }

    public native String initApplicationNative();

    public native String setOkPermission();

    public native String setClosedSoftKeyboard();

    public native String updateScreenSize();

    public native String enableTextInputForPhysicalKeyboard();

    public native String disableTextInputForPhysicalKeyboard();
}

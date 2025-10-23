package me.andreasmelone.autokeyboard.pojav;

import android.content.Context;
import android.support.annotation.Keep;
import android.util.Log;
import android.view.inputmethod.InputMethodManager;

import net.kdt.pojavlaunch.MainActivity;
import net.kdt.pojavlaunch.Tools;
import net.kdt.pojavlaunch.customcontrols.keyboard.TouchCharInput;

@Keep
@SuppressWarnings("unused")
public class AutoKeyboardAndroid {
    @Keep
    public static void setKeyboardOpen(boolean state) {
        try {
            TouchCharInput tcinput = MainActivity.touchCharInput;
            Tools.runOnUiThread(() -> {
                InputMethodManager imm = (InputMethodManager) tcinput.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
                if (!state) {
                    tcinput.clear();
                    tcinput.disable();
                } else {
                    tcinput.enable();
                    imm.showSoftInput(tcinput, InputMethodManager.SHOW_IMPLICIT);
                }
            });
        } catch (Exception e) {
            Log.e("PojIntegr", "Something went wrong", e);
        }
    }
}
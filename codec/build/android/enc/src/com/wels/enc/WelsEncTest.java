package com.wels.enc;

import android.app.Activity;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;

import android.widget.Button;
import android.widget.TextView;
import java.io.*;
import java.util.Vector;

public class WelsEncTest extends Activity {
    /** Called when the activity is first created. */
    private OnClickListener OnClickEvent;
    private Button mBtnLoad, mBtnStartSW;

    final String   mStreamPath = "/sdcard/wels-seq/";
    Vector<String> mCfgFiles = new Vector<String>();

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final TextView  tv = new TextView(this);
        System.out.println("Here we go ...");
        Log.i(TAG, "sdcard path:" + Environment.getExternalStorageDirectory().getAbsolutePath());
        setContentView(R.layout.main);

        mBtnLoad = (Button)findViewById(R.id.cfg);
        mBtnStartSW = (Button)findViewById(R.id.buttonSW);


        OnClickEvent = new OnClickListener()
        {
            public void onClick(View v)
            {
                switch(v.getId())
                {
                case R.id.cfg:
                {
                    String cfgFile = mStreamPath + "cfgs.txt";
                    try {
                        BufferedReader bufferedReader = new BufferedReader(new FileReader(cfgFile));
                        String text;
                        while((text = bufferedReader.readLine()) != null) {
                            mCfgFiles.add(mStreamPath + text);
                            Log.i(TAG, mStreamPath + text);
                        }
                        bufferedReader.close();
                    } catch(IOException e) {
                        Log.e(TAG, e.getMessage());
                    }
                }
                break;
                case R.id.buttonSW:
                {
                    System.out.println("decode sequence number = " + mCfgFiles.size());
                    Log.i(TAG,"after click");
                    try {
                        for (int k=0; k < mCfgFiles.size(); k++) {
                            String cfgFile =  mCfgFiles.get(k);
                            DoEncoderTest(cfgFile);
                        }
                    } catch (Exception e) {
                        Log.e(TAG, e.getMessage());
                    }
                    mCfgFiles.clear();
                    tv.setText( "Decoder is completed!" );
                }
                break;
                }
            }
        };

        mBtnLoad.setOnClickListener(OnClickEvent);
        mBtnStartSW.setOnClickListener(OnClickEvent);

        System.out.println("Done!");
    }

    @Override
    public void onStart()
    {
        Log.i(TAG,"welsdecdemo onStart");
        super.onStart();
    }


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        switch (keyCode) {
        case KeyEvent.KEYCODE_BACK:
            return true;
        default:
            return super.onKeyDown(keyCode, event);
        }
    }

    public native void  DoEncoderTest(String cfgFileName);
    private static final String TAG = "welsenc";
    static {
        try {
            System.loadLibrary("wels");
            System.loadLibrary("stlport_shared");
            System.loadLibrary("welsencdemo");
            Log.v(TAG, "Load libwelsencdemo.so successful");
        }
        catch(Exception e) {
            Log.e(TAG, "Failed to load welsdec"+e.getMessage());
        }
    }

}


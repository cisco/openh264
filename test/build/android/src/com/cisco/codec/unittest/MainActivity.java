package com.cisco.codec.unittest;


import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.util.Log;
import android.widget.TextView;
import android.os.Build;
import android.os.Process;

public class MainActivity extends Activity {

  private TextView mStatusView;

  @Override
  protected void onCreate (Bundle savedInstanceState) {
    super.onCreate (savedInstanceState);
    setContentView (R.layout.activity_main);

    mStatusView = (TextView)findViewById (R.id.status_view);

    runUnitTest();

  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    Log.i ("codec_unittest", "OnDestroy");
    Process.killProcess (Process.myPid());
  }



  public void runUnitTest() {
    Thread thread = new Thread() {

      public void run() {
        Log.i ("codec_unittest", "codec unittest begin");
        CharSequence text = "Running...";
        if (mStatusView != null) {
          mStatusView.setText (text);
        }

//        String path = getIntent().getStringExtra("path");
//        if (path.length() <=0)
//        {
//          path = "/sdcard/codec_unittest.xml";
//        }
        String path = "/sdcard/codec_unittest.xml";
        Log.i ("codec_unittest", "codec unittest runing @" + path);
        DoUnittest ("/sdcard", path);
        Log.i ("codec_unittest", "codec unittest end");
        finish();
      }

    };
    thread.start();
  }

  static {
    try {
      System.loadLibrary ("stlport_shared");
      //System.loadLibrary("openh264");
      System.loadLibrary ("ut");
      System.loadLibrary ("utDemo");


    } catch (Exception e) {
      Log.v ("codec_unittest", "Load library failed");
    }

  }

  public native void DoUnittest (String directory, String path);

}

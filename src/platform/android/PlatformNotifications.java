// Java side of platform_notification_* (platform_android.cpp calls it through JNI).
// Local notifications on Android need Java: a delayed one is an AlarmManager alarm that fires a BroadcastReceiver, which posts
// the notification. Add this file to the app module and declare the receiver in AndroidManifest.xml:
//
//   <uses-permission android:name="android.permission.POST_NOTIFICATIONS"/>     <!-- Android 13+, asked at runtime by add() -->
//   <application ...>
//       <receiver android:name="platform.PlatformNotifications$Receiver" android:exported="false"/>
//   </application>
//
// Alarms are inexact (setAndAllowWhileIdle): no SCHEDULE_EXACT_ALARM permission, the system may delay them a little in Doze.
package platform;

import android.Manifest;
import android.app.Activity;
import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;

public final class PlatformNotifications {
    private static final String CHANNEL_ID = "platform_default";
    private static final String EXTRA_ID = "platform.notification.id";
    private static final String EXTRA_MESSAGE = "platform.notification.message";

    private PlatformNotifications() {}

    private static PendingIntent pending(Context context, String id, String message) {
        Intent intent = new Intent(context, Receiver.class);
        intent.putExtra(EXTRA_ID, id);
        if (message != null) intent.putExtra(EXTRA_MESSAGE, message);
        int flags = PendingIntent.FLAG_UPDATE_CURRENT | (Build.VERSION.SDK_INT >= 23 ? PendingIntent.FLAG_IMMUTABLE : 0);
        return PendingIntent.getBroadcast(context, id.hashCode(), intent, flags);
    }

    // Same id replaces the pending notification, so this is also "update".
    public static void add(Activity activity, long secondsFromNow, String message, String id) {
        if (Build.VERSION.SDK_INT >= 33 && activity.checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED)
            activity.requestPermissions(new String[] { Manifest.permission.POST_NOTIFICATIONS }, 0);

        AlarmManager alarms = (AlarmManager) activity.getSystemService(Context.ALARM_SERVICE);
        PendingIntent pi = pending(activity, id, message);
        alarms.cancel(pi);
        long when = System.currentTimeMillis() + Math.max(1, secondsFromNow) * 1000L;
        if (Build.VERSION.SDK_INT >= 23) alarms.setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, when, pi);
        else                             alarms.set(AlarmManager.RTC_WAKEUP, when, pi);
    }

    // Cancels the pending alarm and removes the notification if it is already shown.
    public static void remove(Activity activity, String id) {
        AlarmManager alarms = (AlarmManager) activity.getSystemService(Context.ALARM_SERVICE);
        alarms.cancel(pending(activity, id, null));
        ((NotificationManager) activity.getSystemService(Context.NOTIFICATION_SERVICE)).cancel(id.hashCode());
    }

    public static final class Receiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String id = intent.getStringExtra(EXTRA_ID);
            String message = intent.getStringExtra(EXTRA_MESSAGE);
            if (id == null || message == null) return;

            NotificationManager manager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
            CharSequence title = context.getApplicationInfo().loadLabel(context.getPackageManager());

            Notification.Builder builder;
            if (Build.VERSION.SDK_INT >= 26) {
                manager.createNotificationChannel(new NotificationChannel(CHANNEL_ID, title, NotificationManager.IMPORTANCE_DEFAULT));
                builder = new Notification.Builder(context, CHANNEL_ID);
            } else {
                builder = new Notification.Builder(context);
            }
            Intent launch = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
            PendingIntent open = PendingIntent.getActivity(context, 0, launch, PendingIntent.FLAG_UPDATE_CURRENT | (Build.VERSION.SDK_INT >= 23 ? PendingIntent.FLAG_IMMUTABLE : 0));

            builder.setSmallIcon(context.getApplicationInfo().icon)
                   .setContentTitle(title)
                   .setContentText(message)
                   .setContentIntent(open)
                   .setAutoCancel(true);
            manager.notify(id.hashCode(), builder.build());
        }
    }
}

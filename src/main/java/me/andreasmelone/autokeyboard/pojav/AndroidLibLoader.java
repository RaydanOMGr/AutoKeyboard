package me.andreasmelone.autokeyboard.pojav;

import me.andreasmelone.autokeyboard.Util;

import java.io.File;

public class AndroidLibLoader {
    public static final AndroidLibLoader INSTANCE = new AndroidLibLoader();

    private String path = "";

    public String get(String name) {
        File file = new File(path + "/extracted_library/lib" + name + ".so");
        //noinspection ResultOfMethodCallIgnored
        file.getParentFile().mkdirs();
        String pathInJar = "/natives/" + getArch() + "/lib" + name + ".so";
        Util.extractFile(pathInJar, file);
        return file.getPath();
    }

    public String getPath() {
        return path;
    }

    public void setPath(String path) {
        this.path = path;
    }

    private static String getArch() {
        String arch = System.getProperty("os.arch").toLowerCase();
        return switch (arch) {
            case "x86" -> "x86";
            case "amd64", "x86_64" -> "x86_64";
            case "arm", "armv7", "armv7l" -> "armeabi-v7a";
            case "aarch64", "arm64" -> "arm64-v8a";
            default -> throw new UnsupportedOperationException("Unknown architecture: " + arch);
        };
    }
}

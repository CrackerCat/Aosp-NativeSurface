# How To Build it

- Set AOSP Env, first.
    ```shell
    source build/envsetup.sh
    # choice which you need
    lunch 2
    ```
- Move sources to `AOSP/external`.
    ```shell
  mv aosp_native_surface_xx path/to/AOSP/external/
  ```
- Just build this module
    ```shell
  mmm external/aosp_native_surface_xx
    ```
- Find target on out folder
    ```shell
    find out -name "*Ssage.*"
    ```

> 因为AOSP 10～13 有一些细节上的变动<br>
> 所以源码都单独分开了<br>
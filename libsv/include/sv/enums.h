#ifndef INCLUDE_SV_ENUM_H
#define INCLUDE_SV_ENUM_H

enum SV_ENUMS
{
    // API
    SV_API_BUFFERCOUNT = 1,         /**< Enum for number of allocated v4l2 buffers. Default value is 10 */
    SV_API_FETCHBLOCKING,           /**< Enum for GetImage modes. Default value is 1 (true) */
    SV_API_BLOCKINGTIMEOUT,         /**< Enum for GetImage timeout. Default value is 0 ms (indefinitely) */
    SV_API_PREVENT_TEARING,         /**< Enum for toggling image tearing prevention mechanism. Default value is 1 (true) */
    SV_API_PREVENT_TEARING_TIMEOUT, /**< Enum for tearing prevention mechanism timeout. Units in ms. 0 ms is indefinite wait. */

    // V4L2
    SV_V4L2_IMAGEFORMAT = 10,       /**< Enum for image format. This is handled by V4L2. */
    SV_V4L2_FRAMESIZE,              /**< Enum for frame size. This is handled by V4L2. */
    SV_V4L2_FRAMEINTERVAL,          /**< Enum for frame interval. This is handled by V4L2.*/
};

enum SV_PLATFORM_PROCESSING
{
    SV_ALGORITHM_AUTODETECT         /**< Processing algorithm is detected automatically based on platform and image format */
};

enum SV_PLATFORM
{
    SV_PLATFORM_JETSON_TX2,         /**< Jetson TX2 */
    SV_PLATFORM_JETSON_XAVIER,      /**< Jetson Xavier */
    SV_PLATFORM_JETSON_NANO,        /**< Jetson Nano */
    SV_PLATFORM_JETSON_XAVIER_NX,   /**< Jetson Xavier NX */
    SV_PLATFORM_JETSON_TX2NX,      /**< Jetson TX2NX */
    SV_PLATFORM_JETSON_AGX_ORIN,    /**< Jetson AGX Orin*/
    SV_PLATFORM_JETSON_ORIN_NANO,    /**< Jetson Orin Nano*/
    SV_PLATFORM_JETSON_ORIN_NX,    /**< Jetson Orin NX*/
    SV_PLATFORM_DRAGONBOARD_410C,   /**< DragonBoard 410c */
    SV_PLATFORM_UNKNOWN             /**< Unable to detect platform */
};

#endif /*INCLUDE_SV_ENUM_H*/

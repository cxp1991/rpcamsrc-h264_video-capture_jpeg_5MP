
/* Generated data (by glib-mkenums) */

#include "gstrpicam-enum-types.h"

/* enumerations from "gstrpicam_types.h" */
#include "gstrpicam_types.h"

GType
gst_rpi_cam_src_exposure_mode_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_OFF,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_OFF",
			  "off" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_AUTO,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_AUTO",
			  "auto" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_NIGHT,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_NIGHT",
			  "night" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_NIGHTPREVIEW,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_NIGHTPREVIEW",
			  "nightpreview" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_BACKLIGHT,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_BACKLIGHT",
			  "backlight" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_SPOTLIGHT,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_SPOTLIGHT",
			  "spotlight" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_SPORTS,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_SPORTS",
			  "sports" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_SNOW,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_SNOW",
			  "snow" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_BEACH,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_BEACH",
			  "beach" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_VERYLONG,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_VERYLONG",
			  "verylong" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_FIXEDFPS,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_FIXEDFPS",
			  "fixedfps" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_ANTISHAKE,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_ANTISHAKE",
			  "antishake" },
			{ GST_RPI_CAM_SRC_EXPOSURE_MODE_FIREWORKS,
			  "GST_RPI_CAM_SRC_EXPOSURE_MODE_FIREWORKS",
			  "fireworks" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("GstRpiCamSrcExposureMode"),
				values);
	}
	return the_type;
}

GType
gst_rpi_cam_src_exposure_metering_mode_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ GST_RPI_CAM_SRC_EXPOSURE_METERING_MODE_AVERAGE,
			  "GST_RPI_CAM_SRC_EXPOSURE_METERING_MODE_AVERAGE",
			  "average" },
			{ GST_RPI_CAM_SRC_EXPOSURE_METERING_MODE_SPOT,
			  "GST_RPI_CAM_SRC_EXPOSURE_METERING_MODE_SPOT",
			  "spot" },
			{ GST_RPI_CAM_SRC_EXPOSURE_METERING_MODE_BACKLIST,
			  "GST_RPI_CAM_SRC_EXPOSURE_METERING_MODE_BACKLIST",
			  "backlist" },
			{ GST_RPI_CAM_SRC_EXPOSURE_METERING_MODE_MATRIX,
			  "GST_RPI_CAM_SRC_EXPOSURE_METERING_MODE_MATRIX",
			  "matrix" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("GstRpiCamSrcExposureMeteringMode"),
				values);
	}
	return the_type;
}

GType
gst_rpi_cam_src_awb_mode_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ GST_RPI_CAM_SRC_AWB_MODE_OFF,
			  "GST_RPI_CAM_SRC_AWB_MODE_OFF",
			  "off" },
			{ GST_RPI_CAM_SRC_AWB_MODE_AUTO,
			  "GST_RPI_CAM_SRC_AWB_MODE_AUTO",
			  "auto" },
			{ GST_RPI_CAM_SRC_AWB_MODE_SUNLIGHT,
			  "GST_RPI_CAM_SRC_AWB_MODE_SUNLIGHT",
			  "sunlight" },
			{ GST_RPI_CAM_SRC_AWB_MODE_CLOUDY,
			  "GST_RPI_CAM_SRC_AWB_MODE_CLOUDY",
			  "cloudy" },
			{ GST_RPI_CAM_SRC_AWB_MODE_SHADE,
			  "GST_RPI_CAM_SRC_AWB_MODE_SHADE",
			  "shade" },
			{ GST_RPI_CAM_SRC_AWB_MODE_TUNGSTEN,
			  "GST_RPI_CAM_SRC_AWB_MODE_TUNGSTEN",
			  "tungsten" },
			{ GST_RPI_CAM_SRC_AWB_MODE_FLUORESCENT,
			  "GST_RPI_CAM_SRC_AWB_MODE_FLUORESCENT",
			  "fluorescent" },
			{ GST_RPI_CAM_SRC_AWB_MODE_INCANDESCENT,
			  "GST_RPI_CAM_SRC_AWB_MODE_INCANDESCENT",
			  "incandescent" },
			{ GST_RPI_CAM_SRC_AWB_MODE_FLASH,
			  "GST_RPI_CAM_SRC_AWB_MODE_FLASH",
			  "flash" },
			{ GST_RPI_CAM_SRC_AWB_MODE_HORIZON,
			  "GST_RPI_CAM_SRC_AWB_MODE_HORIZON",
			  "horizon" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("GstRpiCamSrcAWBMode"),
				values);
	}
	return the_type;
}

GType
gst_rpi_cam_src_image_effect_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ GST_RPI_CAM_SRC_IMAGEFX_NONE,
			  "GST_RPI_CAM_SRC_IMAGEFX_NONE",
			  "none" },
			{ GST_RPI_CAM_SRC_IMAGEFX_NEGATIVE,
			  "GST_RPI_CAM_SRC_IMAGEFX_NEGATIVE",
			  "negative" },
			{ GST_RPI_CAM_SRC_IMAGEFX_SOLARIZE,
			  "GST_RPI_CAM_SRC_IMAGEFX_SOLARIZE",
			  "solarize" },
			{ GST_RPI_CAM_SRC_IMAGEFX_POSTERIZE,
			  "GST_RPI_CAM_SRC_IMAGEFX_POSTERIZE",
			  "posterize" },
			{ GST_RPI_CAM_SRC_IMAGEFX_WHITEBOARD,
			  "GST_RPI_CAM_SRC_IMAGEFX_WHITEBOARD",
			  "whiteboard" },
			{ GST_RPI_CAM_SRC_IMAGEFX_BLACKBOARD,
			  "GST_RPI_CAM_SRC_IMAGEFX_BLACKBOARD",
			  "blackboard" },
			{ GST_RPI_CAM_SRC_IMAGEFX_SKETCH,
			  "GST_RPI_CAM_SRC_IMAGEFX_SKETCH",
			  "sketch" },
			{ GST_RPI_CAM_SRC_IMAGEFX_DENOISE,
			  "GST_RPI_CAM_SRC_IMAGEFX_DENOISE",
			  "denoise" },
			{ GST_RPI_CAM_SRC_IMAGEFX_EMBOSS,
			  "GST_RPI_CAM_SRC_IMAGEFX_EMBOSS",
			  "emboss" },
			{ GST_RPI_CAM_SRC_IMAGEFX_OILPAINT,
			  "GST_RPI_CAM_SRC_IMAGEFX_OILPAINT",
			  "oilpaint" },
			{ GST_RPI_CAM_SRC_IMAGEFX_HATCH,
			  "GST_RPI_CAM_SRC_IMAGEFX_HATCH",
			  "hatch" },
			{ GST_RPI_CAM_SRC_IMAGEFX_GPEN,
			  "GST_RPI_CAM_SRC_IMAGEFX_GPEN",
			  "gpen" },
			{ GST_RPI_CAM_SRC_IMAGEFX_PASTEL,
			  "GST_RPI_CAM_SRC_IMAGEFX_PASTEL",
			  "pastel" },
			{ GST_RPI_CAM_SRC_IMAGEFX_WATERCOLOUR,
			  "GST_RPI_CAM_SRC_IMAGEFX_WATERCOLOUR",
			  "watercolour" },
			{ GST_RPI_CAM_SRC_IMAGEFX_FILM,
			  "GST_RPI_CAM_SRC_IMAGEFX_FILM",
			  "film" },
			{ GST_RPI_CAM_SRC_IMAGEFX_BLUR,
			  "GST_RPI_CAM_SRC_IMAGEFX_BLUR",
			  "blur" },
			{ GST_RPI_CAM_SRC_IMAGEFX_SATURATION,
			  "GST_RPI_CAM_SRC_IMAGEFX_SATURATION",
			  "saturation" },
			{ GST_RPI_CAM_SRC_IMAGEFX_COLOURSWAP,
			  "GST_RPI_CAM_SRC_IMAGEFX_COLOURSWAP",
			  "colourswap" },
			{ GST_RPI_CAM_SRC_IMAGEFX_WASHEDOUT,
			  "GST_RPI_CAM_SRC_IMAGEFX_WASHEDOUT",
			  "washedout" },
			{ GST_RPI_CAM_SRC_IMAGEFX_POSTERISE,
			  "GST_RPI_CAM_SRC_IMAGEFX_POSTERISE",
			  "posterise" },
			{ GST_RPI_CAM_SRC_IMAGEFX_COLOURPOINT,
			  "GST_RPI_CAM_SRC_IMAGEFX_COLOURPOINT",
			  "colourpoint" },
			{ GST_RPI_CAM_SRC_IMAGEFX_COLOURBALANCE,
			  "GST_RPI_CAM_SRC_IMAGEFX_COLOURBALANCE",
			  "colourbalance" },
			{ GST_RPI_CAM_SRC_IMAGEFX_CARTOON,
			  "GST_RPI_CAM_SRC_IMAGEFX_CARTOON",
			  "cartoon" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("GstRpiCamSrcImageEffect"),
				values);
	}
	return the_type;
}

GType
gst_rpi_cam_src_flicker_avoidance_get_type (void)
{
	static GType the_type = 0;
	
	if (the_type == 0)
	{
		static const GEnumValue values[] = {
			{ GST_RPI_CAM_SRC_FLICKERAVOID_OFF,
			  "GST_RPI_CAM_SRC_FLICKERAVOID_OFF",
			  "off" },
			{ GST_RPI_CAM_SRC_FLICKERAVOID_AUTO,
			  "GST_RPI_CAM_SRC_FLICKERAVOID_AUTO",
			  "auto" },
			{ GST_RPI_CAM_SRC_FLICKERAVOID_50HZ,
			  "GST_RPI_CAM_SRC_FLICKERAVOID_50HZ",
			  "50hz" },
			{ GST_RPI_CAM_SRC_FLICKERAVOID_60HZ,
			  "GST_RPI_CAM_SRC_FLICKERAVOID_60HZ",
			  "60hz" },
			{ 0, NULL, NULL }
		};
		the_type = g_enum_register_static (
				g_intern_static_string ("GstRpiCamSrcFlickerAvoidance"),
				values);
	}
	return the_type;
}


/* Generated data ends here */


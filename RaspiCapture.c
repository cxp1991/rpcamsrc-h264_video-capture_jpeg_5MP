/*
 * Copyright (c) 2013 Jan Schmidt <jan@centricular.com>
Portions:
Copyright (c) 2013, Broadcom Europe Ltd
Copyright (c) 2013, James Hughes
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * \file RaspiCapture.c
 *
 * Modification of the RaspiVid command line capture program for GStreamer
 * use.
 *
 * \date 28th Feb 2013, 11 Oct 2013
 * \Author: James Hughes, Jan Schmidt
 *
 * Description
 *
 * 3 components are created; camera, preview and video encoder.
 * Camera component has three ports, preview, video and stills.
 * This program connects preview and stills to the preview and video
 * encoder. Using mmal we don't need to worry about buffers between these
 * components, but we do need to handle buffers from the encoder, which
 * are simply written straight to the file in the requisite buffer callback.
 *
 * We use the RaspiCamControl code to handle the specific camera settings.
 * We use the RaspiPreview code to handle the (generic) preview window
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sysexits.h>

#include <gst/gst.h>

#include "bcm_host.h"
#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/mmal/mmal_parameters_camera.h"

#include "RaspiCapture.h"
#include "RaspiCamControl.h"
#include "RaspiPreview.h"

#include <semaphore.h>

/// Camera number to use - we only have one camera, indexed from 0.
#define CAMERA_NUMBER 0

// Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

// Video format information
#define VIDEO_FRAME_RATE_NUM 30
#define VIDEO_FRAME_RATE_DEN 1

/// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3

// Max bitrate we allow for recording
const int MAX_BITRATE = 30000000;	// 30Mbits/s

/// Interval at which we check for an failure abort during capture
const int ABORT_INTERVAL = 100;	// ms

#define MAX_USER_EXIF_TAGS      32
#define MAX_EXIF_PAYLOAD_LENGTH 128

int mmal_status_to_int(MMAL_STATUS_T status);

/** Struct used to pass information in encoder port userdata to callback
 */
typedef struct {
	FILE *file_handle;
	RASPIVID_STATE *state;	/// pointer to our state in case required in callback
	VCOS_SEMAPHORE_T complete_semaphore;
	int abort;		/// Set to 1 in callback if an error occurs to attempt to abort the capture
} PORT_USERDATA;

struct RASPIVID_STATE_T {
	RASPIVID_CONFIG *config;

	FILE *output_file;

	/* File for save image jpeg */
	char *image_filename;
	const char *exifTags[MAX_USER_EXIF_TAGS];	/// Array of pointers to tags supplied from the command line
	int numExifTags;	/// Number of supplied tags
	int enableExifTags;
	RASPICAM_CAMERA_PARAMETERS camera_parameters;	/// Camera setup parameters

	MMAL_COMPONENT_T *camera_component;	/// Pointer to the camera component
	MMAL_COMPONENT_T *encoder_component;	/// Pointer to the encoder component
	MMAL_COMPONENT_T *encoder_capture_component;

	MMAL_CONNECTION_T *preview_connection;	/// Pointer to the connection from camera to preview
	MMAL_CONNECTION_T *encoder_connection;	/// Pointer to the connection from camera to encoder
	MMAL_CONNECTION_T *encoder_capture_connection;

	MMAL_PORT_T *camera_video_port;
	MMAL_PORT_T *camera_still_port;
	MMAL_PORT_T *encoder_output_port;
	MMAL_PORT_T *encoder_capture_output_port;

	MMAL_POOL_T *encoder_pool;	/// Pointer to the pool of buffers used by encoder output port
	MMAL_POOL_T *encoder_capture_pool;

	PORT_USERDATA callback_data;

	MMAL_QUEUE_T *encoded_buffer_q;
};

#if 0
/// Structure to cross reference H264 profile strings against the MMAL parameter equivalent
static XREF_T profile_map[] = {
	{"baseline", MMAL_VIDEO_PROFILE_H264_BASELINE},
	{"main", MMAL_VIDEO_PROFILE_H264_MAIN},
	{"high", MMAL_VIDEO_PROFILE_H264_HIGH},
//   {"constrained",  MMAL_VIDEO_PROFILE_H264_CONSTRAINED_BASELINE} // Does anyone need this?
};

static int profile_map_size = sizeof(profile_map) / sizeof(profile_map[0]);

static void display_valid_parameters(char *app_name);

/// Command ID's and Structure defining our command line options
#define CommandHelp         0
#define CommandWidth        1
#define CommandHeight       2
#define CommandBitrate      3
#define CommandOutput       4
#define CommandVerbose      5
#define CommandTimeout      6
#define CommandDemoMode     7
#define CommandFramerate    8
#define CommandPreviewEnc   9
#define CommandIntraPeriod  10
#define CommandProfile      11

static COMMAND_LIST cmdline_commands[] = {
	{CommandHelp, "-help", "?", "This help information", 0},
	{CommandWidth, "-width", "w", "Set image width <size>. Default 1920",
	 1},
	{CommandHeight, "-height", "h", "Set image height <size>. Default 1080",
	 1},
	{CommandBitrate, "-bitrate", "b",
	 "Set bitrate. Use bits per second (e.g. 10MBits/s would be -b 10000000)",
	 1},
	{CommandVerbose, "-verbose", "v",
	 "Output verbose information during run", 0},
	{CommandTimeout, "-timeout", "t",
	 "Time (in ms) to capture for. If not specified, set to 5s. Zero to disable",
	 1},
	{CommandDemoMode, "-demo", "d",
	 "Run a demo mode (cycle through range of camera options, no capture)",
	 1},
	{CommandFramerate, "-framerate", "fps",
	 "Specify the frames per second to record", 1},
	{CommandPreviewEnc, "-penc", "e",
	 "Display preview image *after* encoding (shows compression artifacts)",
	 0},
	{CommandIntraPeriod, "-intra", "g",
	 "Specify the intra refresh period (key frame rate/GoP size)", 1},
	{CommandProfile, "-profile", "pf",
	 "Specify H264 profile to use for encoding", 1},
};

static int cmdline_commands_size = sizeof(cmdline_commands) / sizeof(cmdline_commands[0]);
#endif

static void dump_state(RASPIVID_STATE * state);

/**
 * Assign a default set of parameters to the state passed in
 *
 * @param state Pointer to state structure to assign defaults to
 */
void raspicapture_default_config(RASPIVID_CONFIG * config)
{

	// Now set anything non-zero
	config->timeout = 5000;	// 5s delay before take image
	config->width = 1920;	// Default to 1080p
	config->height = 1080;
	config->bitrate = 17000000;	// This is a decent default bitrate for 1080p
	config->fps_n = VIDEO_FRAME_RATE_NUM;
	config->fps_d = VIDEO_FRAME_RATE_DEN;
	config->intraperiod = 0;	// Not set
	config->demoMode = 0;
	config->demoInterval = 250;	// ms
	config->immutableInput = 1;
	config->profile = MMAL_VIDEO_PROFILE_H264_HIGH;

	// Setup preview window defaults
	raspipreview_set_defaults(&config->preview_parameters);

	// Set up the camera_parameters to default
	raspicamcontrol_set_defaults(&config->camera_parameters);

}

/**
 * Dump image state parameters to printf. Used for debugging
 *
 * @param state Pointer to state structure to assign defaults to
 */
static void dump_state(RASPIVID_STATE * state)
{
	if (!state) {
		vcos_assert(0);
		return;
	}

	fprintf(stderr, "Width %d, Height %d\n", state->config->width, state->config->height);
	fprintf(stderr, "bitrate %d, framerate %d/%d, time delay %d\n",
		state->config->bitrate, state->config->fps_n, state->config->fps_d,
		state->config->timeout);
	//fprintf(stderr, "H264 Profile %s\n", raspicli_unmap_xref(state->config->profile, profile_map, profile_map_size));

	raspipreview_dump_parameters(&state->config->preview_parameters);
	raspicamcontrol_dump_parameters(&state->config->camera_parameters);
}

/**
 *  buffer header callback function for camera control
 *
 *  Callback will dump buffer data to the specific file
 *
 * @param port Pointer to port from which callback originated
 * @param buffer mmal buffer header pointer
 */
static void camera_control_callback(MMAL_PORT_T * port, MMAL_BUFFER_HEADER_T * buffer)
{
	puts("camera_control_callback");

	if (buffer->cmd == MMAL_EVENT_PARAMETER_CHANGED) {
	} else {
		vcos_log_error("Received unexpected camera control callback event, 0x%08x",
			       buffer->cmd);
	}

	mmal_buffer_header_release(buffer);
}

/** [image]
 *  buffer header callback function for encoder
 *
 *  Callback will dump buffer data to the specific file
 *
 * @param port Pointer to port from which callback originated
 * @param buffer mmal buffer header pointer
 */
static void encoder_capture_buffer_callback(MMAL_PORT_T * port, MMAL_BUFFER_HEADER_T * buffer)
{
	//puts("encoder_capture_buffer_callback");
	int complete = 0;
	static int count = 0;

	// We pass our file handle and other stuff in via the userdata field.

	PORT_USERDATA *pData = (PORT_USERDATA *) port->userdata;

	if (pData) {
		int bytes_written = buffer->length;

		if (buffer->length && pData->file_handle) {
			mmal_buffer_header_mem_lock(buffer);

			bytes_written = fwrite(buffer->data, 1, buffer->length, pData->file_handle);
			printf
			    ("\n buffer->data = %d, buffer->length = %d, pData->file_handle = %d\n",
			     buffer->data, buffer->length, pData->file_handle);

			mmal_buffer_header_mem_unlock(buffer);
		}
		// We need to check we wrote what we wanted - it's possible we have run out of storage.
		if (bytes_written != buffer->length) {
			vcos_log_error("Unable to write buffer to file - aborting");
			complete = 1;
		}
		// Now flag if we have completed
		if (buffer->
		    flags & (MMAL_BUFFER_HEADER_FLAG_FRAME_END |
			     MMAL_BUFFER_HEADER_FLAG_TRANSMISSION_FAILED))
			complete = 1;
		if (!complete)
			count++;
	} else {
		vcos_log_error("Received a encoder buffer callback with no state");
	}

	// release buffer back to the pool
	mmal_buffer_header_release(buffer);

	// and send one back to the port (if still open)
	if (port->is_enabled) {
		MMAL_STATUS_T status = MMAL_SUCCESS;
		MMAL_BUFFER_HEADER_T *new_buffer;

		new_buffer = mmal_queue_get(pData->state->encoder_capture_pool->queue);

		if (new_buffer) {
			status = mmal_port_send_buffer(port, new_buffer);
		}
		if (!new_buffer || status != MMAL_SUCCESS)
			vcos_log_error("Unable to return a buffer to the encoder port");
	}

	if (complete) {
		//printf("\ncount = %d\n", count);
		//puts("semaphore post");
		vcos_semaphore_post(&(pData->complete_semaphore));
	}
}

/**
 *  buffer header callback function for encoder
 *
 *  Callback will dump buffer data to the specific file
 *
 * @param port Pointer to port from which callback originated
 * @param buffer mmal buffer header pointer
 */
static void encoder_buffer_callback(MMAL_PORT_T * port, MMAL_BUFFER_HEADER_T * buffer)
{
	//puts("encoder_buffer_callback");
	PORT_USERDATA *pData = (PORT_USERDATA *) port->userdata;
	RASPIVID_STATE *state = pData->state;

	if (pData == NULL) {
		vcos_log_error("Received a encoder buffer callback with no state");
		// release buffer back to the pool
		mmal_buffer_header_release(buffer);
		return;
	}

	/* Send buffer to GStreamer element for pushing to the pipeline */
	mmal_queue_put(state->encoded_buffer_q, buffer);
}

GstFlowReturn raspi_capture_fill_buffer(RASPIVID_STATE * state, GstBuffer ** bufp)
{
	// puts("raspi_capture_fill_buffer");
	GstBuffer *buf;
	MMAL_BUFFER_HEADER_T *buffer;
	GstFlowReturn ret = GST_FLOW_ERROR;

	/* FIXME: Use our own interruptible cond wait: */
	buffer = mmal_queue_wait(state->encoded_buffer_q);

	mmal_buffer_header_mem_lock(buffer);
	buf = gst_buffer_new_allocate(NULL, buffer->length, NULL);
	if (buf) {
		gst_buffer_fill(buf, 0, buffer->data, buffer->length);
		ret = GST_FLOW_OK;
	}

	mmal_buffer_header_mem_unlock(buffer);

	*bufp = buf;
	// release buffer back to the pool
	mmal_buffer_header_release(buffer);

	// and send one back to the port (if still open)
	if (state->encoder_output_port->is_enabled) {
		MMAL_STATUS_T status = MMAL_SUCCESS;

		buffer = mmal_queue_get(state->encoder_pool->queue);
		if (buffer)
			status = mmal_port_send_buffer(state->encoder_output_port, buffer);

		if (!buffer || status != MMAL_SUCCESS) {
			vcos_log_error("Unable to return a buffer to the encoder port");
			ret = GST_FLOW_ERROR;
		}
	}

	return ret;
}

/**
 * Create the camera component, set up its ports
 *
 * @param state Pointer to state control struct
 *
 * @return MMAL_SUCCESS if all OK, something else otherwise
 *
 */
static MMAL_STATUS_T create_camera_component(RASPIVID_STATE * state)
{
	puts("create_camera_component");
	MMAL_COMPONENT_T *camera = NULL;
	MMAL_STATUS_T status;

	/* Create the component */
	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Failed to create camera component");
		goto error;
	}

	if (!camera->output_num) {
		status = MMAL_ENOSYS;
		vcos_log_error("Camera doesn't have output ports");
		goto error;
	}
	// Enable the camera, and tell it its control callback function
	status = mmal_port_enable(camera->control, camera_control_callback);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Unable to enable control port : error %d", status);
		goto error;
	}

	state->camera_component = camera;

	return status;

 error:
	if (camera)
		mmal_component_destroy(camera);

	return status;
}

MMAL_STATUS_T raspi_capture_set_format_and_start(RASPIVID_STATE * state)
{
	puts("raspi_capture_set_format_and_start");
	MMAL_COMPONENT_T *camera = NULL;
	MMAL_STATUS_T status;
	MMAL_ES_FORMAT_T *format;
	MMAL_PORT_T *preview_port = NULL, *video_port = NULL, *still_port = NULL;

	//  set up the camera configuration

	printf("\nstate->config->width = %d\n"
	       "state->config->height = %d\n", state->config->width, state->config->height);

	MMAL_PARAMETER_CAMERA_CONFIG_T cam_config = {
		{MMAL_PARAMETER_CAMERA_CONFIG, sizeof(cam_config)}
		,
		.max_stills_w = 2592,
		.max_stills_h = 1944,
		.stills_yuv422 = 0,
		.one_shot_stills = 1,
		.max_preview_video_w = state->config->width,
		.max_preview_video_h = state->config->height,
		.num_preview_video_frames = 3,
		.stills_capture_circular_buffer_height = 0,
		.fast_preview_resume = 0,
		.use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC
	};

	camera = state->camera_component;
	preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];
	video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];
	still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];

	mmal_port_parameter_set(camera->control, &cam_config.hdr);

	// Now set up the port formats

	// Set the encode format on the Preview port
	// HW limitations mean we need the preview to be the same size as the required recorded output

	format = preview_port->format;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->encoding_variant = MMAL_ENCODING_I420;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->es->video.width = state->config->width;
	format->es->video.height = state->config->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = state->config->width;
	format->es->video.crop.height = state->config->height;
	format->es->video.frame_rate.num = state->config->fps_n;
	format->es->video.frame_rate.den = state->config->fps_d;

	status = mmal_port_format_commit(preview_port);
	vcos_assert(status == MMAL_SUCCESS);

	// Set the encode format on the video  port

	format = video_port->format;
	format->encoding_variant = MMAL_ENCODING_I420;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->es->video.width = state->config->width;
	format->es->video.height = state->config->height;
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = state->config->width;
	format->es->video.crop.height = state->config->height;
	format->es->video.frame_rate.num = state->config->fps_n;
	format->es->video.frame_rate.den = state->config->fps_d;

	status = mmal_port_format_commit(video_port);
	vcos_assert(status == MMAL_SUCCESS);

	// Ensure there are enough buffers to avoid dropping frames
	if (video_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		video_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

	// Set the encode format on the still  port

	format = still_port->format;

	format->encoding = MMAL_ENCODING_OPAQUE;
	format->es->video.width = VCOS_ALIGN_UP(2592, 32);
	format->es->video.height = VCOS_ALIGN_UP(1944, 16);
	format->es->video.crop.x = 0;
	format->es->video.crop.y = 0;
	format->es->video.crop.width = 2592;
	format->es->video.crop.height = 1944;
	format->es->video.frame_rate.num = 0;
	format->es->video.frame_rate.den = 1;

	status = mmal_port_format_commit(still_port);
	vcos_assert(status == MMAL_SUCCESS);

	/* Ensure there are enough buffers to avoid dropping frames */
	if (still_port->buffer_num < VIDEO_OUTPUT_BUFFERS_NUM)
		still_port->buffer_num = VIDEO_OUTPUT_BUFFERS_NUM;

	/* Enable component */
	status = mmal_component_enable(camera);
	vcos_assert(status == MMAL_SUCCESS);

	raspicamcontrol_set_all_parameters(camera, &state->config->camera_parameters);

	if (state->config->verbose)
		fprintf(stderr, "Camera component done\n");

 error:
	return status;
}

/**
 * Destroy the camera component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroy_camera_component(RASPIVID_STATE * state)
{
	if (state->camera_component) {
		mmal_component_destroy(state->camera_component);
		state->camera_component = NULL;
	}
}

/**
 * cxphong esit
 *
 * Create the encoder capture component, set up its ports
 *
 * @param state Pointer to state control struct. encoder_component member set to the created camera_component if successfull.
 *
 * @return a MMAL_STATUS, MMAL_SUCCESS if all OK, something else otherwise
 */
static MMAL_STATUS_T create_encoder_capture_component(RASPIVID_STATE * state)
{
	MMAL_COMPONENT_T *encoder = 0;
	MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL;
	MMAL_STATUS_T status;
	MMAL_POOL_T *pool;

	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER, &encoder);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Unable to create JPEG encoder component");
		goto error;
	}

	if (!encoder->input_num || !encoder->output_num) {
		status = MMAL_ENOSYS;
		vcos_log_error("JPEG encoder doesn't have input/output ports");
		goto error;
	}

	encoder_input = encoder->input[0];
	encoder_output = encoder->output[0];

	// We want same format on input and output
	mmal_format_copy(encoder_output->format, encoder_input->format);

	// Specify out output format
	encoder_output->format->encoding = MMAL_ENCODING_JPEG;

	encoder_output->buffer_size = encoder_output->buffer_size_recommended;

	if (encoder_output->buffer_size < encoder_output->buffer_size_min)
		encoder_output->buffer_size = encoder_output->buffer_size_min;

	encoder_output->buffer_num = encoder_output->buffer_num_recommended;

	if (encoder_output->buffer_num < encoder_output->buffer_num_min)
		encoder_output->buffer_num = encoder_output->buffer_num_min;

	// Commit the port changes to the output port
	status = mmal_port_format_commit(encoder_output);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Unable to set format on video encoder output port");
		goto error;
	}
	// Set the JPEG quality level
	status = mmal_port_parameter_set_uint32(encoder_output, MMAL_PARAMETER_JPEG_Q_FACTOR, 85);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Unable to set JPEG quality");
		goto error;
	}
	// Set up any required thumbnail
	{
		MMAL_PARAMETER_THUMBNAIL_CONFIG_T param_thumb =
		    { {MMAL_PARAMETER_THUMBNAIL_CONFIGURATION,
		       sizeof(MMAL_PARAMETER_THUMBNAIL_CONFIG_T)}
		, 0, 0, 0, 0
		};

		//if ( state->thumbnailConfig.enable &&
		//    state->thumbnailConfig.width > 0 && state->thumbnailConfig.height > 0 )
		//{
		// Have a valid thumbnail defined
		param_thumb.enable = 1;
		param_thumb.width = 64;
		param_thumb.height = 48;
		param_thumb.quality = 35;
		//}
		status = mmal_port_parameter_set(encoder->control, &param_thumb.hdr);
	}

	//  Enable component
	status = mmal_component_enable(encoder);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Unable to enable video encoder component");
		goto error;
	}

	/* Create pool of buffer headers for the output port to consume */
	pool =
	    mmal_port_pool_create(encoder_output, encoder_output->buffer_num,
				  encoder_output->buffer_size);

	if (!pool) {
		vcos_log_error("Failed to create buffer header pool for encoder output port %s",
			       encoder_output->name);
	}

	state->encoder_capture_pool = pool;
	state->encoder_capture_component = encoder;

	if (state->config->verbose)
		fprintf(stderr, "Encoder component done\n");

	return status;

 error:

	if (encoder)
		mmal_component_destroy(encoder);

	return status;
}

/**
 * Create the encoder component, set up its ports
 *
 * @param state Pointer to state control struct
 *
 * @return MMAL_SUCCESS if all OK, something else otherwise
 *
 */
static MMAL_STATUS_T create_encoder_component(RASPIVID_STATE * state)
{
	puts("create_encoder_component");
	MMAL_COMPONENT_T *encoder = 0;
	MMAL_PORT_T *encoder_input = NULL, *encoder_output = NULL;
	MMAL_STATUS_T status;
	MMAL_POOL_T *pool;

	status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER, &encoder);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Unable to create video encoder component");
		goto error;
	}

	if (!encoder->input_num || !encoder->output_num) {
		status = MMAL_ENOSYS;
		vcos_log_error("Video encoder doesn't have input/output ports");
		goto error;
	}

	encoder_input = encoder->input[0];
	encoder_output = encoder->output[0];

	// We want same format on input and output
	mmal_format_copy(encoder_output->format, encoder_input->format);

	// Only supporting H264 at the moment
	encoder_output->format->encoding = MMAL_ENCODING_H264;

	encoder_output->format->bitrate = state->config->bitrate;

	encoder_output->buffer_size = encoder_output->buffer_size_recommended;

	if (encoder_output->buffer_size < encoder_output->buffer_size_min)
		encoder_output->buffer_size = encoder_output->buffer_size_min;

	GST_DEBUG("encoder buffer size is %u", (guint) encoder_output->buffer_size);

	encoder_output->buffer_num = encoder_output->buffer_num_recommended;

	if (encoder_output->buffer_num < encoder_output->buffer_num_min)
		encoder_output->buffer_num = encoder_output->buffer_num_min;

	// Commit the port changes to the output port
	status = mmal_port_format_commit(encoder_output);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Unable to set format on video encoder output port");
		goto error;
	}
	// Set the rate control parameter
	if (0) {
		MMAL_PARAMETER_VIDEO_RATECONTROL_T param =
		    { {MMAL_PARAMETER_RATECONTROL, sizeof(param)}
		, MMAL_VIDEO_RATECONTROL_DEFAULT
		};
		status = mmal_port_parameter_set(encoder_output, &param.hdr);
		if (status != MMAL_SUCCESS) {
			vcos_log_error("Unable to set ratecontrol");
			goto error;
		}

	}

	if (state->config->intraperiod) {
		MMAL_PARAMETER_UINT32_T param = { {MMAL_PARAMETER_INTRAPERIOD, sizeof(param)}
		, state->config->intraperiod
		};
		status = mmal_port_parameter_set(encoder_output, &param.hdr);
		if (status != MMAL_SUCCESS) {
			vcos_log_error("Unable to set intraperiod");
			goto error;
		}

	}

	{
		MMAL_PARAMETER_VIDEO_PROFILE_T param;
		param.hdr.id = MMAL_PARAMETER_PROFILE;
		param.hdr.size = sizeof(param);

		param.profile[0].profile = state->config->profile;
		param.profile[0].level = MMAL_VIDEO_LEVEL_H264_4;	// This is the only value supported

		status = mmal_port_parameter_set(encoder_output, &param.hdr);
		if (status != MMAL_SUCCESS) {
			vcos_log_error("Unable to set H264 profile");
			goto error;
		}
	}

	if (mmal_port_parameter_set_boolean
	    (encoder_input, MMAL_PARAMETER_VIDEO_IMMUTABLE_INPUT,
	     state->config->immutableInput) != MMAL_SUCCESS) {
		vcos_log_error("Unable to set immutable input flag");
		// Continue rather than abort..
	}
	//  Enable component
	status = mmal_component_enable(encoder);

	if (status != MMAL_SUCCESS) {
		vcos_log_error("Unable to enable video encoder component");
		goto error;
	}

	/* Create pool of buffer headers for the output port to consume */
	pool =
	    mmal_port_pool_create(encoder_output, encoder_output->buffer_num,
				  encoder_output->buffer_size);
	if (!pool) {
		vcos_log_error("Failed to create buffer header pool for encoder output port %s",
			       encoder_output->name);
	}

	state->encoder_pool = pool;
	state->encoder_component = encoder;

	if (state->config->verbose)
		fprintf(stderr, "Encoder component done\n");

	return status;

 error:
	if (encoder)
		mmal_component_destroy(encoder);

	return status;
}

/**
 * Destroy the encoder component
 *
 * @param state Pointer to state control struct
 *
 */
static void destroy_encoder_component(RASPIVID_STATE * state)
{
	/* Empty the buffer header q */
	while (mmal_queue_length(state->encoded_buffer_q)) {
		MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state->encoded_buffer_q);
		mmal_buffer_header_release(buffer);
	}
	mmal_queue_destroy(state->encoded_buffer_q);

	// Get rid of any port buffers first
	if (state->encoder_pool) {
		mmal_port_pool_destroy(state->encoder_component->output[0], state->encoder_pool);
	}

	if (state->encoder_component) {
		mmal_component_destroy(state->encoder_component);
		state->encoder_component = NULL;
	}
}

/**
 * Connect two specific ports together
 *
 * @param output_port Pointer the output port
 * @param input_port Pointer the input port
 * @param Pointer to a mmal connection pointer, reassigned if function successful
 * @return Returns a MMAL_STATUS_T giving result of operation
 *
 */
static MMAL_STATUS_T connect_ports(MMAL_PORT_T * output_port, MMAL_PORT_T * input_port,
				   MMAL_CONNECTION_T ** connection)
{
	MMAL_STATUS_T status;

	status =
	    mmal_connection_create(connection, output_port, input_port,
				   MMAL_CONNECTION_FLAG_TUNNELLING |
				   MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);

	if (status == MMAL_SUCCESS) {
		status = mmal_connection_enable(*connection);
		if (status != MMAL_SUCCESS)
			mmal_connection_destroy(*connection);
	}

	return status;
}

/**
 * Checks if specified port is valid and enabled, then disables it
 *
 * @param port  Pointer the port
 *
 */
static void check_disable_port(MMAL_PORT_T * port)
{
	if (port && port->is_enabled)
		mmal_port_disable(port);
}

void raspicapture_init()
{
	bcm_host_init();

	// Register our application with the logging system
	vcos_log_register("RaspiVid", VCOS_LOG_CATEGORY);
}

static void rename_file(RASPIVID_STATE * state, FILE * output_file, const char *final_filename,
			const char *use_filename, int frame)
{
	MMAL_STATUS_T status;

	fclose(output_file);
	vcos_assert(use_filename != NULL && final_filename != NULL);
	if (0 != rename(use_filename, final_filename)) {
		vcos_log_error("Could not rename temp file to: %s; %s", final_filename,
			       strerror(errno));
	}
}

/**
 * Add an exif tag to the capture
 *
 * @param state Pointer to state control struct
 * @param exif_tag String containing a "key=value" pair.
 * @return  Returns a MMAL_STATUS_T giving result of operation
 */
static MMAL_STATUS_T add_exif_tag(RASPIVID_STATE * state, const char *exif_tag)
{
	MMAL_STATUS_T status;
	MMAL_PARAMETER_EXIF_T *exif_param =
	    (MMAL_PARAMETER_EXIF_T *) calloc(sizeof(MMAL_PARAMETER_EXIF_T) +
					     MAX_EXIF_PAYLOAD_LENGTH, 1);

	vcos_assert(state);
	vcos_assert(state->encoder_capture_component);

	// Check to see if the tag is present or is indeed a key=value pair.
	if (!exif_tag || strchr(exif_tag, '=') == NULL
	    || strlen(exif_tag) > MAX_EXIF_PAYLOAD_LENGTH - 1)
		return MMAL_EINVAL;

	exif_param->hdr.id = MMAL_PARAMETER_EXIF;

	strncpy((char *)exif_param->data, exif_tag, MAX_EXIF_PAYLOAD_LENGTH - 1);

	exif_param->hdr.size = sizeof(MMAL_PARAMETER_EXIF_T) + strlen((char *)exif_param->data);

	status =
	    mmal_port_parameter_set(state->encoder_capture_component->output[0], &exif_param->hdr);

	free(exif_param);

	return status;
}

/**
 * Add a basic set of EXIF tags to the capture
 * Make, Time etc
 *
 * @param state Pointer to state control struct
 *
 */
static void add_exif_tags(RASPIVID_STATE * state)
{
	time_t rawtime;
	struct tm *timeinfo;
	char time_buf[32];
	char exif_buf[128];
	int i;

	add_exif_tag(state, "IFD0.Model=RP_OV5647");
	add_exif_tag(state, "IFD0.Make=RaspberryPi");

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	snprintf(time_buf, sizeof(time_buf),
		 "%04d:%02d:%02d %02d:%02d:%02d",
		 timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
		 timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	snprintf(exif_buf, sizeof(exif_buf), "EXIF.DateTimeDigitized=%s", time_buf);
	add_exif_tag(state, exif_buf);

	snprintf(exif_buf, sizeof(exif_buf), "EXIF.DateTimeOriginal=%s", time_buf);
	add_exif_tag(state, exif_buf);

	snprintf(exif_buf, sizeof(exif_buf), "IFD0.DateTime=%s", time_buf);
	add_exif_tag(state, exif_buf);

	// Now send any user supplied tags

	for (i = 0; i < state->numExifTags && i < MAX_USER_EXIF_TAGS; i++) {
		if (state->exifTags[i]) {
			add_exif_tag(state, state->exifTags[i]);
		}
	}
}

/**
 * Allocates and generates a filename based on the
 * user-supplied pattern and the frame number.
 * On successful return, finalName and tempName point to malloc()ed strings
 * which must be freed externally.  (On failure, returns nulls that
 * don't need free()ing.)
 *
 * @param finalName pointer receives an
 * @param pattern sprintf pattern with %d to be replaced by frame
 * @param frame for timelapse, the frame number
 * @return Returns a MMAL_STATUS_T giving result of operation
 */

MMAL_STATUS_T create_filenames(char **finalName, char **tempName, char *pattern, int frame)
{
	*finalName = NULL;
	*tempName = NULL;
	if (0 > asprintf(finalName, pattern, frame) || 0 > asprintf(tempName, "%s~", *finalName)) {
		if (*finalName != NULL) {
			free(*finalName);
		}
		return MMAL_ENOMEM;	// It may be some other error, but it is not worth getting it right
	}
	return MMAL_SUCCESS;
}

/**
 * raspi_capture_image:
 *
 * Capture image and save as jpeg format
 */
gboolean raspi_capture_image(RASPIVID_STATE * state, const char *filename)
{
	int frame = 0;
	FILE *output_file = NULL;
	char *use_filename = NULL;	// Temporary filename while image being written
	char *final_filename = NULL;	// Name that file gets once writing complete
	MMAL_STATUS_T status = MMAL_SUCCESS;
	VCOS_STATUS_T vcos_status;

	/* Stop encoder of video h264 */
	//status = mmal_port_disable(state->encoder_output_port);
	//mmal_port_parameter_set_boolean(state->camera_video_port, MMAL_PARAMETER_CAPTURE, 0);

	/* Open the file */
	if (filename) {
		vcos_assert(use_filename == NULL && final_filename == NULL);

		/* Create file name */
		status = create_filenames(&final_filename, &use_filename, filename, frame);

		if (status != MMAL_SUCCESS) {
			vcos_log_error("Unable to create filenames");
			vcos_assert(0);
		}

		output_file = fopen(use_filename, "wb");
		vcos_assert(output_file);
		//}

		state->callback_data.file_handle = output_file;
	}

	int num, q;

	add_exif_tags(state);

	/* Enable the encoder output port */
	state->encoder_capture_output_port->userdata =
	    (struct MMAL_PORT_USERDATA_T *)&state->callback_data;

	/* Enable the encoder output port and tell it its callback function */
	status =
	    mmal_port_enable(state->encoder_capture_output_port, encoder_capture_buffer_callback);
	vcos_assert(status == MMAL_SUCCESS);

	vcos_status =
	    vcos_semaphore_create(&(state->callback_data.complete_semaphore), "RaspiStill-sem", 0);
	vcos_assert(status == MMAL_SUCCESS);

	/* Send all the buffers to the encoder output port */
	num = mmal_queue_length(state->encoder_capture_pool->queue);

	for (q = 0; q < num; q++) {
		puts("for loop [image]");
		MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state->encoder_capture_pool->queue);
		vcos_assert(buffer);

		status = mmal_port_send_buffer(state->encoder_capture_output_port, buffer);
		vcos_assert(status == MMAL_SUCCESS);
	}

	status =
	    mmal_port_parameter_set_boolean(state->camera_still_port, MMAL_PARAMETER_CAPTURE, 1);
	vcos_assert(status == MMAL_SUCCESS);

	/* Wait until capture image done */
	vcos_semaphore_wait(&(state->callback_data.complete_semaphore));

	/* Ensure we don't die if get callback with no open file */
	state->callback_data.file_handle = NULL;

	if (output_file != stdout) {
		rename_file(&state, output_file, final_filename, use_filename, frame);
	} else {
		fflush(output_file);
	}

	/* Disable encoder output port */
	status = mmal_port_disable(state->encoder_capture_output_port);

	/* Enable encoder of video h264 */
	//status = mmal_port_enable(state->encoder_output_port, encoder_buffer_callback);
	// mmal_port_parameter_set_boolean(state->camera_video_port, MMAL_PARAMETER_CAPTURE, 0);

	//}

	if (use_filename) {
		free(use_filename);
		use_filename = NULL;
	}
	if (final_filename) {
		free(final_filename);
		final_filename = NULL;
	}

	vcos_semaphore_delete(&(state->callback_data.complete_semaphore));

	vcos_semaphore_delete(&(state->callback_data.complete_semaphore));

}

static RASPIVID_STATE *mState;

static void capture_image_setup(RASPIVID_STATE * state)
{
	puts("capture_image_setup()");

	MMAL_PORT_T *encoder_capture_input_port = NULL;
	MMAL_STATUS_T status = MMAL_SUCCESS;

	/* Create jpeg encoder */
	if ((status = create_encoder_capture_component(state)) != MMAL_SUCCESS) {
		vcos_log_error("%s: Failed to create encode capture component", __func__);
		raspipreview_destroy(&state->config->preview_parameters);
		destroy_camera_component(state);
		return NULL;
	}

	/* Connect camera capture port to jpeg encoder */
	encoder_capture_input_port = state->encoder_capture_component->input[0];
	state->encoder_capture_output_port = state->encoder_capture_component->output[0];
	status =
	    connect_ports(state->camera_still_port, encoder_capture_input_port,
			  &state->encoder_capture_connection);
	vcos_assert(status == MMAL_SUCCESS);
}

char *raspi_capture_photo()
{
	puts("RaspiCapture.c[line 1119]: raspberry_capture_photo");

	/** 
	 * Create file's name
	 * File name format: ddmmyy_hhmmss.jpg
	 */
	static char name[100] = { 0 };

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	sprintf(name, "%d%d%d_%d%d%d.jpg", tm.tm_mday, tm.tm_mon + 1,
		tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);

	/* Initialize capture image component */
	capture_image_setup(mState);

	/* Start capture image */
	raspi_capture_image(mState, name);

	return name;
}

/**
 * raspi_capture_setup:
 *
 * Create all components: camera, preview, h264 encoder, jpeg encoder
 */
RASPIVID_STATE *raspi_capture_setup(RASPIVID_CONFIG * config)
{
	puts("raspi_capture_setup");
	RASPIVID_STATE *state;

	MMAL_STATUS_T status = MMAL_SUCCESS;

	/* Default everything to zero */
	state = calloc(1, sizeof(RASPIVID_STATE));

	/* Apply passed in config */
	state->config = config;

	/* Create camera component */
	if ((status = create_camera_component(state)) != MMAL_SUCCESS) {
		vcos_log_error("%s: Failed to create camera component", __func__);
		return NULL;
	}

	/* Create preview */
	if ((status = raspipreview_create(&state->config->preview_parameters)) != MMAL_SUCCESS) {
		vcos_log_error("%s: Failed to create preview component", __func__);
		destroy_camera_component(state);
		return NULL;
	}

	/* Create h264 encoder */
	if ((status = create_encoder_component(state)) != MMAL_SUCCESS) {
		vcos_log_error("%s: Failed to create encode component", __func__);
		raspipreview_destroy(&state->config->preview_parameters);
		destroy_camera_component(state);
		return NULL;
	}

	/* Create jpeg encoder */
	/*if ((status = create_encoder_capture_component(state)) != MMAL_SUCCESS) {
	   vcos_log_error("%s: Failed to create encode capture component", __func__);
	   raspipreview_destroy(&state->config->preview_parameters);
	   destroy_camera_component(state);
	   return NULL;
	   } */

	/* Config camera components */
	status = raspi_capture_set_format_and_start(state);
	vcos_assert(status == MMAL_SUCCESS);

	/* Create queue to hold data from encoder video h264 port, then send to gstreamer */
	state->encoded_buffer_q = mmal_queue_create();

	return state;
}

/**
 * raspi_capture_start:
 *
 * Connect components together.
 * Enable video callback
 */
 /* Listen capture image */
 
gboolean raspi_capture_start(RASPIVID_STATE * state)
{
	MMAL_STATUS_T status = MMAL_SUCCESS;
	MMAL_PORT_T *camera_preview_port = NULL;
	MMAL_PORT_T *preview_input_port = NULL;
	MMAL_PORT_T *encoder_input_port = NULL;
	if (state->config->verbose) {
		dump_state(state);
	}
	if ((status = raspi_capture_set_format_and_start(state)) != MMAL_SUCCESS) {
		return FALSE;
	}
	if (state->config->verbose)
		fprintf(stderr, "Starting component connection stage\n");
	camera_preview_port = state->camera_component->output[MMAL_CAMERA_PREVIEW_PORT];
	preview_input_port = state->config->preview_parameters.preview_component->input[0];
	encoder_input_port = state->encoder_component->input[0];
	state->camera_video_port = state->camera_component->output[MMAL_CAMERA_VIDEO_PORT];
	state->camera_still_port = state->camera_component->output[MMAL_CAMERA_CAPTURE_PORT];
	state->encoder_output_port = state->encoder_component->output[0];
	if (state->config->preview_parameters.wantPreview) {
		if (state->config->verbose) {
			fprintf(stderr, "Connecting camera preview port to preview input port\n");
			fprintf(stderr, "Starting video preview\n");
		}
// Connect camera to preview
		status =
		    connect_ports(camera_preview_port, preview_input_port,
				  &state->preview_connection);
		if (status != MMAL_SUCCESS) {
			vcos_log_error("%s: Failed to connect camera to preview", __func__);
			return FALSE;
		}
	}
	if (state->config->verbose)
		fprintf(stderr, "Connecting camera stills port to encoder input port\n");
// Now connect the camera to the encoder
	status =
	    connect_ports(state->camera_video_port, encoder_input_port, &state->encoder_connection);
	if (status != MMAL_SUCCESS) {
		if (state->config->preview_parameters.wantPreview)
			mmal_connection_destroy(state->preview_connection);
		vcos_log_error("%s: Failed to connect camera video port to encoder input",
			       __func__);
		return FALSE;
	}
// Set up our userdata - this is passed though to the callback where we need the information.
	state->callback_data.state = state;
	state->callback_data.abort = 0;
	state->encoder_output_port->userdata = (struct MMAL_PORT_USERDATA_T *)&state->callback_data;
	if (state->config->verbose)
		fprintf(stderr, "Enabling encoder output port\n");
// Enable the encoder output port and tell it its callback function
	status = mmal_port_enable(state->encoder_output_port, encoder_buffer_callback);
	if (status != MMAL_SUCCESS) {
		vcos_log_error("Failed to setup encoder output");
		goto error;
	}
	if (state->config->demoMode) {
// Run for the user specific time..
		int num_iterations = state->config->timeout / state->config->demoInterval;
		int i;
		if (state->config->verbose)
			fprintf(stderr, "Running in demo mode\n");
		for (i = 0; state->config->timeout == 0 || i < num_iterations; i++) {
			raspicamcontrol_cycle_test(state->camera_component);
			vcos_sleep(state->config->demoInterval);
		}
	}
	/* Save state for capture photo  */
	mState = calloc(1, sizeof(RASPIVID_STATE));
	mState = state;

	if (state->config->verbose)
		fprintf(stderr, "Starting video capture\n");
	if (mmal_port_parameter_set_boolean(state->camera_video_port, MMAL_PARAMETER_CAPTURE, 1) !=
	    MMAL_SUCCESS) {
		goto error;
	}
// Send all the buffers to the encoder output port
	{
		int num = mmal_queue_length(state->encoder_pool->queue);
		int q;
		for (q = 0; q < num; q++) {
			MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(state->encoder_pool->queue);
			if (!buffer)
				vcos_log_error("Unable to get a required buffer %d from pool queue",
					       q);
			if (mmal_port_send_buffer(state->encoder_output_port, buffer) !=
			    MMAL_SUCCESS)
				vcos_log_error
				    ("Unable to send a buffer to encoder output port (%d)", q);
		}
	}
// Now wait until we need to stop. Whilst waiting we do need to check to see if we have aborted (for example
// out of storage space)
// Going to check every ABORT_INTERVAL milliseconds
#if 0
	for (wait = 0; state->config->timeout == 0 || wait < state->config->timeout;
	     wait += ABORT_INTERVAL) {
		vcos_sleep(ABORT_INTERVAL);
		if (state->callback_data.abort)
			break;
	}
	if (state->config->verbose)
		fprintf(stderr, "Finished capture\n");
#endif
	return (status == MMAL_SUCCESS);
 error:
	raspi_capture_stop(state);
	if (status != MMAL_SUCCESS) {
		mmal_status_to_int(status);
		raspicamcontrol_check_configuration(128);
	}
	return FALSE;
}

void raspi_capture_stop(RASPIVID_STATE * state)
{
	if (state->config->verbose)
		fprintf(stderr, "Closing down\n");

	if (state->config->preview_parameters.wantPreview)
		mmal_connection_destroy(state->preview_connection);
	mmal_connection_destroy(state->encoder_connection);

	/* Disable all our ports that are not handled by connections */
	check_disable_port(state->camera_still_port);
	check_disable_port(state->encoder_output_port);
}

void raspi_capture_free(RASPIVID_STATE * state)
{
	// Can now close our file. Note disabling ports may flush buffers which causes
	// problems if we have already closed the file!
	if (state->output_file && state->output_file != stdout)
		fclose(state->output_file);

	/* Disable components */
	if (state->encoder_component)
		mmal_component_disable(state->encoder_component);

	if (state->config->preview_parameters.preview_component)
		mmal_component_disable(state->config->preview_parameters.preview_component);

	if (state->camera_component)
		mmal_component_disable(state->camera_component);

	destroy_encoder_component(state);
	raspipreview_destroy(&state->config->preview_parameters);
	destroy_camera_component(state);

	if (state->config->verbose)
		fprintf(stderr,
			"Close down completed, all components disconnected, disabled and destroyed\n\n");

	free(state);
}

package endpoints

import (
	"fmt"
	"net/http"
	"time"

	"github.com/ZeusyBoy98/SnackYou/state"
	"github.com/gin-gonic/gin"
)

const cameraBoundary = "123456789000000000000987654321"

func GetCameraStream(c *gin.Context) {
	c.Header("Content-Type", "multipart/x-mixed-replace;boundary="+cameraBoundary)
	c.Header("Cache-Control", "no-cache, no-store, must-revalidate")
	c.Header("Pragma", "no-cache")
	c.Header("Expires", "0")
	c.Header("X-Accel-Buffering", "no")

	flusher, ok := c.Writer.(http.Flusher)
	if !ok {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "streaming unsupported"})
		return
	}

	var lastSeq uint64
	for {
		select {
		case <-c.Request.Context().Done():
			return
		default:
		}

		state.CameraMu.RLock()
		seq := state.CameraLatest.Seq
		frameData := append([]byte(nil), state.CameraLatest.Data...)
		state.CameraMu.RUnlock()

		if len(frameData) == 0 || seq == lastSeq {
			time.Sleep(100 * time.Millisecond)
			continue
		}

		if _, err := fmt.Fprintf(c.Writer, "\r\n--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n",
			cameraBoundary, len(frameData)); err != nil {
			return
		}
		if _, err := c.Writer.Write(frameData); err != nil {
			return
		}
		flusher.Flush()

		lastSeq = seq
	}
}

package esp_endpoints

import (
	"io"
	"net/http"
	"time"

	"github.com/ZeusyBoy98/SnackYou/state"
	"github.com/gin-gonic/gin"
)

const maxCameraFrameBytes = 512 * 1024

func PostCameraFrame(c *gin.Context) {
	frame, err := io.ReadAll(io.LimitReader(c.Request.Body, maxCameraFrameBytes+1))
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	if len(frame) == 0 {
		c.JSON(http.StatusBadRequest, gin.H{"error": "empty frame"})
		return
	}
	if len(frame) > maxCameraFrameBytes {
		c.JSON(http.StatusRequestEntityTooLarge, gin.H{"error": "frame too large"})
		return
	}

	state.CameraMu.Lock()
	state.CameraLatest.Data = append(state.CameraLatest.Data[:0], frame...)
	state.CameraLatest.Seq++
	state.CameraLatest.UpdatedAt = time.Now()
	seq := state.CameraLatest.Seq
	state.CameraMu.Unlock()

	c.JSON(http.StatusOK, gin.H{"ok": true, "seq": seq})
}

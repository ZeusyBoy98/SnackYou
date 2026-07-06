package state

import (
	"sync"
	"time"
)

type CameraFrame struct {
	Data      []byte
	Seq       uint64
	UpdatedAt time.Time
}

var (
	CameraMu     sync.RWMutex
	CameraLatest CameraFrame
)

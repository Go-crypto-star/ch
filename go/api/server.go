package api

import (
    "net/http"
    "strconv"
    "time"

    "chia-pool"
    "github.com/gin-gonic/gin"
    "github.com/sirupsen/logrus"
)

// Server представляет HTTP API сервер пула
type Server struct {
    router *gin.Engine
    port   int
    log    *logrus.Logger
    pool   *pool.PoolManager
}

// NewServer создает новый экземпляр сервера
func NewServer(port int, poolManager *pool.PoolManager) *Server {
    server := &Server{
        router: gin.New(),
        port:   port,
        log:    logrus.New(),
        pool:   poolManager,
    }
    
    server.setupRoutes()
    server.setupMiddleware()
    
    return server
}

// setupMiddleware настраивает middleware
func (s *Server) setupMiddleware() {
    s.router.Use(gin.Recovery())
    s.router.Use(s.loggingMiddleware())
    s.router.Use(s.corsMiddleware())
    s.router.Use(s.rateLimitMiddleware())
}

// setupRoutes настраивает маршруты API
func (s *Server) setupRoutes() {
    v1 := s.router.Group("/api/v1")
    {
        // Public endpoints
        v1.GET("/pool_info", s.getPoolInfo)
        v1.GET("/stats", s.getStats)
        
        // Farmer endpoints
        farmer := v1.Group("/farmer")
        farmer.Use(s.authMiddleware())
        {
            farmer.POST("/register", s.registerFarmer)
            farmer.GET("/info", s.getFarmerInfo)
            farmer.POST("/partial", s.submitPartial)
        }
        
        // Payout endpoints
        payout := v1.Group("/payout")
        payout.Use(s.authMiddleware())
        {
            payout.GET("/history", s.getPayoutHistory)
            payout.POST("/request", s.requestPayout)
        }
        
        // Admin endpoints
        admin := v1.Group("/admin")
        admin.Use(s.adminAuthMiddleware())
        {
            admin.GET("/farmers", s.getAllFarmers)
            admin.POST("/payouts/process", s.processPayouts)
            admin.GET("/system/stats", s.getSystemStats)
        }
    }
    
    // Health check
    s.router.GET("/health", s.healthCheck)
    s.router.GET("/ready", s.readyCheck)
}

// Start запускает HTTP сервер
func (s *Server) Start() error {
    s.log.Infof("Starting API server on port %d", s.port)
    return s.router.Run(":" + strconv.Itoa(s.port))
}

// getPoolInfo возвращает информацию о пуле
func (s *Server) getPoolInfo(c *gin.Context) {
    poolInfo, err := s.pool.GetPoolInfo()
    if err != nil {
        s.log.Errorf("Failed to get pool info: %v", err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to get pool information",
        })
        return
    }
    
    c.JSON(http.StatusOK, poolInfo)
}

// getStats возвращает статистику пула
func (s *Server) getStats(c *gin.Context) {
    farmers, partials, validPartials, points, err := s.pool.GetStatistics()
    if err != nil {
        s.log.Errorf("Failed to get stats: %v", err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to get pool statistics",
        })
        return
    }
    
    c.JSON(http.StatusOK, gin.H{
        "total_farmers":    farmers,
        "total_partials":   partials,
        "valid_partials":   validPartials,
        "total_points":     points,
        "timestamp":        time.Now().Unix(),
    })
}

// registerFarmer регистрирует нового фермера
func (s *Server) registerFarmer(c *gin.Context) {
    var request struct {
        LauncherID string `json:"launcher_id" binding:"required,len=64"`
        PoolURL    string `json:"pool_url" binding:"required,url"`
    }
    
    if err := c.ShouldBindJSON(&request); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{
            "error": "Invalid request: " + err.Error(),
        })
        return
    }
    
    farmer := pool.Farmer{
        LauncherID: request.LauncherID,
        PoolURL:    request.PoolURL,
        Timestamp:  time.Now(),
        IsActive:   true,
    }
    
    if err := s.pool.AddFarmer(farmer); err != nil {
        s.log.Errorf("Failed to register farmer %s: %v", request.LauncherID, err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to register farmer",
        })
        return
    }
    
    c.JSON(http.StatusOK, gin.H{
        "status":   "success",
        "message":  "Farmer registered successfully",
        "farmer":   farmer,
    })
}

// getFarmerInfo возвращает информацию о фермере
func (s *Server) getFarmerInfo(c *gin.Context) {
    launcherID := c.GetString("launcher_id")
    
    farmer, err := s.pool.GetFarmer(launcherID)
    if err != nil {
        s.log.Errorf("Failed to get farmer info for %s: %v", launcherID, err)
        c.JSON(http.StatusNotFound, gin.H{
            "error": "Farmer not found",
        })
        return
    }
    
    c.JSON(http.StatusOK, farmer)
}

// submitPartial принимает partial решение от фермера
func (s *Server) submitPartial(c *gin.Context) {
    launcherID := c.GetString("launcher_id")
    
    var request struct {
        Challenge  string `json:"challenge" binding:"required,len=64"`
        Signature  string `json:"signature" binding:"required,len=192"`
        Timestamp  int64  `json:"timestamp" binding:"required"`
        Difficulty uint64 `json:"difficulty" binding:"required"`
        PlotSize   uint32 `json:"plot_size" binding:"required,min=25,max=50"`
    }
    
    if err := c.ShouldBindJSON(&request); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{
            "error": "Invalid request: " + err.Error(),
        })
        return
    }
    
    partial := pool.PartialRequest{
        Challenge:  request.Challenge,
        LauncherID: launcherID,
        Signature:  request.Signature,
        Timestamp:  request.Timestamp,
        Difficulty: request.Difficulty,
        PlotSize:   request.PlotSize,
    }
    
    success, err := s.pool.ProcessPartial(partial)
    if err != nil {
        s.log.Errorf("Failed to process partial for %s: %v", launcherID, err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to process partial",
        })
        return
    }
    
    if !success {
        c.JSON(http.StatusBadRequest, gin.H{
            "error": "Invalid partial solution",
        })
        return
    }
    
    c.JSON(http.StatusOK, gin.H{
        "status":  "success",
        "message": "Partial solution accepted",
    })
}

// getPayoutHistory возвращает историю выплат фермера
func (s *Server) getPayoutHistory(c *gin.Context) {
    launcherID := c.GetString("launcher_id")
    
    history, err := s.pool.GetPayoutHistory(launcherID)
    if err != nil {
        s.log.Errorf("Failed to get payout history for %s: %v", launcherID, err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to get payout history",
        })
        return
    }
    
    c.JSON(http.StatusOK, gin.H{
        "payouts": history,
    })
}

// requestPayout запрашивает выплату
func (s *Server) requestPayout(c *gin.Context) {
    launcherID := c.GetString("launcher_id")
    
    var request struct {
        Amount uint64 `json:"amount,omitempty"`
    }
    
    if err := c.ShouldBindJSON(&request); err != nil {
        c.JSON(http.StatusBadRequest, gin.H{
            "error": "Invalid request: " + err.Error(),
        })
        return
    }
    
    payout, err := s.pool.RequestPayout(launcherID, request.Amount)
    if err != nil {
        s.log.Errorf("Failed to request payout for %s: %v", launcherID, err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to request payout: " + err.Error(),
        })
        return
    }
    
    c.JSON(http.StatusOK, gin.H{
        "status": "success",
        "payout": payout,
    })
}

// getAllFarmers возвращает список всех фермеров (admin only)
func (s *Server) getAllFarmers(c *gin.Context) {
    farmers, err := s.pool.GetAllFarmers()
    if err != nil {
        s.log.Errorf("Failed to get all farmers: %v", err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to get farmers list",
        })
        return
    }
    
    c.JSON(http.StatusOK, gin.H{
        "farmers": farmers,
    })
}

// processPayouts запускает процесс выплат (admin only)
func (s *Server) processPayouts(c *gin.Context) {
    if err := s.pool.ProcessPayouts(); err != nil {
        s.log.Errorf("Failed to process payouts: %v", err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to process payouts: " + err.Error(),
        })
        return
    }
    
    c.JSON(http.StatusOK, gin.H{
        "status":  "success",
        "message": "Payouts processing started",
    })
}

// getSystemStats возвращает системную статистику (admin only)
func (s *Server) getSystemStats(c *gin.Context) {
    stats, err := s.pool.GetSystemStats()
    if err != nil {
        s.log.Errorf("Failed to get system stats: %v", err)
        c.JSON(http.StatusInternalServerError, gin.H{
            "error": "Failed to get system statistics",
        })
        return
    }
    
    c.JSON(http.StatusOK, stats)
}

// healthCheck проверка здоровья сервера
func (s *Server) healthCheck(c *gin.Context) {
    c.JSON(http.StatusOK, gin.H{
        "status":    "healthy",
        "timestamp": time.Now().Unix(),
        "version":   "1.0.0",
    })
}

// readyCheck проверка готовности сервера
func (s *Server) readyCheck(c *gin.Context) {
    if !s.pool.IsReady() {
        c.JSON(http.StatusServiceUnavailable, gin.H{
            "status": "not ready",
        })
        return
    }
    
    c.JSON(http.StatusOK, gin.H{
        "status": "ready",
    })
}

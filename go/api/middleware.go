package api

import (
    "net/http"
    "strings"
    "time"

    "github.com/gin-gonic/gin"
    "github.com/sirupsen/logrus"
    "golang.org/x/time/rate"
)

// Rate limiting storage
var (
    limiter = rate.NewLimiter(rate.Every(time.Minute), 60)
    ipLimiters = make(map[string]*rate.Limiter)
)

// loggingMiddleware логирует все запросы
func (s *Server) loggingMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        start := time.Now()
        
        // Process request
        c.Next()
        
        duration := time.Since(start)
        
        entry := s.log.WithFields(logrus.Fields{
            "client_ip":   c.ClientIP(),
            "method":      c.Request.Method,
            "path":        c.Request.URL.Path,
            "status":      c.Writer.Status(),
            "duration":    duration.String(),
            "user_agent":  c.Request.UserAgent(),
        })
        
        if c.Writer.Status() >= 500 {
            entry.Error("HTTP request error")
        } else if c.Writer.Status() >= 400 {
            entry.Warn("HTTP request client error")
        } else {
            entry.Info("HTTP request")
        }
    }
}

// corsMiddleware настраивает CORS
func (s *Server) corsMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        c.Writer.Header().Set("Access-Control-Allow-Origin", "*")
        c.Writer.Header().Set("Access-Control-Allow-Credentials", "true")
        c.Writer.Header().Set("Access-Control-Allow-Headers", 
            "Content-Type, Content-Length, Accept-Encoding, X-CSRF-Token, Authorization, accept, origin, Cache-Control, X-Requested-With")
        c.Writer.Header().Set("Access-Control-Allow-Methods", 
            "POST, OPTIONS, GET, PUT, DELETE")
        
        if c.Request.Method == "OPTIONS" {
            c.AbortWithStatus(204)
            return
        }
        
        c.Next()
    }
}

// rateLimitMiddleware ограничивает частоту запросов
func (s *Server) rateLimitMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        clientIP := c.ClientIP()
        
        // Get or create limiter for this IP
        limiter, exists := ipLimiters[clientIP]
        if !exists {
            limiter = rate.NewLimiter(rate.Every(time.Minute), 60)
            ipLimiters[clientIP] = limiter
        }
        
        if !limiter.Allow() {
            s.log.Warnf("Rate limit exceeded for IP: %s", clientIP)
            c.JSON(http.StatusTooManyRequests, gin.H{
                "error": "Rate limit exceeded",
            })
            c.Abort()
            return
        }
        
        c.Next()
    }
}

// authMiddleware аутентифицирует фермера
func (s *Server) authMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        authHeader := c.GetHeader("Authorization")
        if authHeader == "" {
            c.JSON(http.StatusUnauthorized, gin.H{
                "error": "Authorization header required",
            })
            c.Abort()
            return
        }
        
        // Check if it's a Bearer token
        parts := strings.Split(authHeader, " ")
        if len(parts) != 2 || parts[0] != "Bearer" {
            c.JSON(http.StatusUnauthorized, gin.H{
                "error": "Invalid authorization format",
            })
            c.Abort()
            return
        }
        
        token := parts[1]
        
        // Validate token and extract launcher_id
        launcherID, err := s.validateToken(token)
        if err != nil {
            s.log.Warnf("Invalid token from IP %s: %v", c.ClientIP(), err)
            c.JSON(http.StatusUnauthorized, gin.H{
                "error": "Invalid token",
            })
            c.Abort()
            return
        }
        
        // Set launcher_id in context
        c.Set("launcher_id", launcherID)
        c.Next()
    }
}

// adminAuthMiddleware аутентифицирует администратора
func (s *Server) adminAuthMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        authHeader := c.GetHeader("Authorization")
        if authHeader == "" {
            c.JSON(http.StatusUnauthorized, gin.H{
                "error": "Authorization header required",
            })
            c.Abort()
            return
        }
        
        // Simple admin token check (in production use proper JWT or similar)
        if authHeader != "Bearer "+ s.pool.GetAdminToken() {
            s.log.Warnf("Invalid admin token attempt from IP: %s", c.ClientIP())
            c.JSON(http.StatusForbidden, gin.H{
                "error": "Admin access required",
            })
            c.Abort()
            return
        }
        
        c.Next()
    }
}

// validateToken валидирует JWT токен и возвращает launcher_id
func (s *Server) validateToken(token string) (string, error) {
    // In a real implementation, this would validate JWT tokens
    // For now, we'll use a simple implementation
    
    if len(token) != 64 {
        return "", fmt.Errorf("invalid token length")
    }
    
    // Simple validation - in production use proper JWT validation
    // This is just a placeholder implementation
    return token, nil
}

// securityHeadersMiddleware добавляет security headers
func (s *Server) securityHeadersMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        c.Writer.Header().Set("X-Content-Type-Options", "nosniff")
        c.Writer.Header().Set("X-Frame-Options", "DENY")
        c.Writer.Header().Set("X-XSS-Protection", "1; mode=block")
        c.Writer.Header().Set("Strict-Transport-Security", "max-age=31536000")
        c.Writer.Header().Set("Content-Security-Policy", 
            "default-src 'self'; script-src 'self' 'unsafe-inline'")
        
        c.Next()
    }
}

// timeoutMiddleware устанавливает timeout для запросов
func (s *Server) timeoutMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        // Set timeout for all requests
        ctx, cancel := context.WithTimeout(c.Request.Context(), 30*time.Second)
        defer cancel()
        
        c.Request = c.Request.WithContext(ctx)
        
        c.Next()
    }
}

// recoverMiddleware обрабатывает panic и возвращает 500
func (s *Server) recoverMiddleware() gin.HandlerFunc {
    return func(c *gin.Context) {
        defer func() {
            if err := recover(); err != nil {
                s.log.Errorf("Panic recovered: %v", err)
                c.JSON(http.StatusInternalServerError, gin.H{
                    "error": "Internal server error",
                })
                c.Abort()
            }
        }()
        
        c.Next()
    }
}

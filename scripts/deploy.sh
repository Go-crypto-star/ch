#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

PROJECT_NAME="pool-reference-cpp"
BUILD_DIR="build"
DEPLOY_DIR="/opt/$PROJECT_NAME"
CONFIG_DIR="/etc/$PROJECT_NAME"
SERVICE_NAME="$PROJECT_NAME"

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "Please run as root for deployment"
        exit 1
    fi
}

check_build() {
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory not found. Run build.sh first."
        exit 1
    fi
    
    local required_files=(
        "$BUILD_DIR/libpool.so"
        "$BUILD_DIR/pool_bridge"
        "$BUILD_DIR/pool_api"
    )
    
    for file in "${required_files[@]}"; do
        if [ ! -f "$file" ]; then
            print_warning "Required file not found: $file"
        fi
    done
}

create_directories() {
    print_info "Creating deployment directories..."
    
    mkdir -p $DEPLOY_DIR/{bin,lib,config,logs}
    mkdir -p $CONFIG_DIR
}

install_binaries() {
    print_info "Installing binaries..."
    
    # Install libraries
    if [ -f "$BUILD_DIR/libpool.so" ]; then
        cp $BUILD_DIR/libpool.so $DEPLOY_DIR/lib/
        ldconfig $DEPLOY_DIR/lib 2>/dev/null || true
    fi
    
    # Install executables
    if [ -f "$BUILD_DIR/pool_bridge" ]; then
        cp $BUILD_DIR/pool_bridge $DEPLOY_DIR/bin/
    fi
    
    if [ -f "$BUILD_DIR/pool_api" ]; then
        cp $BUILD_DIR/pool_api $DEPLOY_DIR/bin/
    fi
    
    # Set executable permissions
    chmod +x $DEPLOY_DIR/bin/* 2>/dev/null || true
}

install_configs() {
    print_info "Installing configuration files..."
    
    if [ -d "config" ]; then
        cp config/*.json $CONFIG_DIR/ 2>/dev/null || true
        
        # Create default config if not exists
        if [ ! -f "$CONFIG_DIR/pool_config.json" ]; then
            cat > $CONFIG_DIR/pool_config.json << EOF
{
    "pool": {
        "name": "$PROJECT_NAME",
        "port": 8444,
        "fee_percentage": 1.0,
        "payout_scheme": "PPLNS"
    },
    "blockchain": {
        "chia_rpc_host": "localhost",
        "chia_rpc_port": 8555,
        "network": "mainnet"
    },
    "api": {
        "enabled": true,
        "port": 8080,
        "rate_limit": 100
    }
}
EOF
        fi
    fi
}

create_service_file() {
    print_info "Creating systemd service..."
    
    cat > /etc/systemd/system/$SERVICE_NAME.service << EOF
[Unit]
Description=$PROJECT_NAME - Chia Pool Reference Implementation
After=network.target

[Service]
Type=simple
User=pool
Group=pool
WorkingDirectory=$DEPLOY_DIR
ExecStart=$DEPLOY_DIR/bin/pool_bridge --config $CONFIG_DIR/pool_config.json
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Security
NoNewPrivileges=yes
PrivateTmp=yes
ProtectSystem=strict
ProtectHome=yes
ReadWritePaths=$DEPLOY_DIR/logs $CONFIG_DIR

[Install]
WantedBy=multi-user.target
EOF
}

create_user() {
    if ! id "pool" &>/dev/null; then
        print_info "Creating 'pool' user..."
        useradd -r -s /bin/false -d $DEPLOY_DIR pool
    fi
}

set_permissions() {
    print_info "Setting permissions..."
    
    chown -R pool:pool $DEPLOY_DIR
    chown -R pool:pool $CONFIG_DIR
    
    chmod 755 $DEPLOY_DIR
    chmod 750 $DEPLOY_DIR/bin/*
    chmod 644 $CONFIG_DIR/*.json
    chmod 755 $DEPLOY_DIR/logs
}

enable_service() {
    print_info "Enabling and starting service..."
    
    systemctl daemon-reload
    systemctl enable $SERVICE_NAME
    systemctl start $SERVICE_NAME
}

setup_logging() {
    print_info "Setting up log rotation..."
    
    cat > /etc/logrotate.d/$PROJECT_NAME << EOF
$DEPLOY_DIR/logs/*.log {
    daily
    missingok
    rotate 7
    compress
    delaycompress
    notifempty
    copytruncate
}
EOF
}

show_status() {
    print_info "Deployment completed!"
    echo
    echo "Service: $SERVICE_NAME"
    echo "Installation directory: $DEPLOY_DIR"
    echo "Configuration directory: $CONFIG_DIR"
    echo
    echo "Useful commands:"
    echo "  systemctl status $SERVICE_NAME"
    echo "  journalctl -u $SERVICE_NAME -f"
    echo "  systemctl restart $SERVICE_NAME"
    echo
    echo "Next steps:"
    echo "1. Review configuration: $CONFIG_DIR/pool_config.json"
    echo "2. Start the service: systemctl start $SERVICE_NAME"
    echo "3. Check logs: journalctl -u $SERVICE_NAME -f"
}

main() {
    local skip_service=false
    local skip_user=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --skip-service)
                skip_service=true
                shift
                ;;
            --skip-user)
                skip_user=true
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    print_info "Starting deployment of $PROJECT_NAME"
    
    check_root
    check_build
    
    create_directories
    install_binaries
    install_configs
    
    if [ "$skip_user" = false ]; then
        create_user
    fi
    
    set_permissions
    
    if [ "$skip_service" = false ]; then
        create_service_file
        setup_logging
        enable_service
    fi
    
    show_status
}

# If script is sourced, don't run main
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi

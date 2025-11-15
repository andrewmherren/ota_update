# OTA Update Module

A complete over-the-air firmware update system for ESP32 devices using the web_platform framework. This module provides enterprise-grade OTA capabilities with multiple authentication modes, semantic versioning, and AWS CloudFormation deployment templates.

## Features

### Core Capabilities
- ✅ **Multiple Authentication Modes**: PUBLIC, SHARED_KEY, DEVICE_SPECIFIC, SIGNED_URLS
- ✅ **Semantic Versioning**: Intelligent update paths with breaking change detection
- ✅ **Web Interface**: Complete HTML5 interface with real-time progress updates
- ✅ **RESTful API**: Full REST API for programmatic control and monitoring
- ✅ **AWS Integration**: CloudFormation templates for scalable infrastructure
- ✅ **Enterprise Ready**: Support from hobbyist (1-5 devices) to commercial (1000+ devices)
- ✅ **Memory Efficient**: PROGMEM assets, fragmentation prevention
- ✅ **Robust Error Handling**: Automatic rollback, verification, retry logic

### Security & Reliability
- ✅ **HTTPS Only**: All communications use secure transport
- ✅ **SHA-256 Verification**: Mandatory firmware integrity checking
- ✅ **Automatic Rollback**: Failed updates automatically rollback
- ✅ **Update History**: Complete audit trail of all update attempts
- ✅ **Development Mode**: Safe testing with downgrade and rollback support

## Quick Start

### 1. Basic Integration

```cpp
#include <web_platform.h>
#include <ota_update_module.h>

void setup() {
    // Initialize web platform
    webPlatform.begin("MyDevice");
    
  // Configure OTA module
    JsonDocument config;
    config["auto_check"] = false;

  // Register module with configuration (platform manages lifecycle)
  webPlatform.registerModule("/ota", &otaUpdateModule, config.as<JsonVariant>());
}

void loop() {
    webPlatform.handle();
}
```

### 2. Build Configuration

```ini
# platformio.ini
[env:esp32]
build_flags = 
  -DOTA_AUTH_MODE=PUBLIC_ACCESS
  -DOTA_FIRMWARE_VERSION='"1.0.0"'
  -DOTA_MANIFEST_URL='"https://your-cdn.cloudfront.net/manifest.json"'
```

### 3. Deploy Infrastructure

Choose the appropriate template for your authentication mode:

```bash
# PUBLIC mode - Development (direct S3 access)
aws cloudformation create-stack \
  --stack-name my-device-ota-dev \
  --template-body file://cloudformation/public-dev.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device \
  --capabilities CAPABILITY_NAMED_IAM

# PUBLIC mode - Production (CloudFront CDN)
aws cloudformation create-stack \
  --stack-name my-device-ota-prod \
  --template-body file://cloudformation/public-prod.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device \
  --capabilities CAPABILITY_NAMED_IAM

# SHARED_KEY mode - Development
aws cloudformation create-stack \
  --stack-name my-device-ota-shared-dev \
  --template-body file://cloudformation/shared-key-dev.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device \
                ParameterKey=SharedKey,ParameterValue=your-secure-key \
  --capabilities CAPABILITY_NAMED_IAM

# SHARED_KEY mode - Production (with Lambda validation)
aws cloudformation create-stack \
  --stack-name my-device-ota-shared-prod \
  --template-body file://cloudformation/shared-key-prod.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device \
                ParameterKey=SharedKey,ParameterValue=your-secure-key \
  --capabilities CAPABILITY_NAMED_IAM

# DEVICE_SPECIFIC mode (with device registry)
aws cloudformation create-stack \
  --stack-name my-device-ota-device \
  --template-body file://cloudformation/device-specific.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device \
                ParameterKey=Environment,ParameterValue=prod \
  --capabilities CAPABILITY_NAMED_IAM
```

### 4. Upload Firmware

```bash
# Generate SHA256 checksum
sha256sum firmware.bin

# Create manifest.json
cat > manifest.json << 'EOF'
{
  "product": "my-device",
  "versions": {
    "1.1.0": {
      "url": "https://your-bucket.s3.us-east-1.amazonaws.com/firmware/v1.1.0.bin",
      "sha256": "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890",
      "released": "2024-01-15T10:30:00Z",
      "size_bytes": 1048576,
      "notes": "Bug fixes and improvements"
    }
  },
  "latest": "1.1.0"
}
EOF

# Retrieve uploader credentials from Secrets Manager (created by CloudFormation)
# These are restricted IAM credentials with upload-only access to the firmware bucket
# Using a named profile avoids interfering with your existing AWS credentials (e.g., SSO)

# For Linux/Mac (bash):
SECRET=$(aws secretsmanager get-secret-value \
  --secret-id my-device-uploader-credentials-dev \
  --query SecretString --output text)
aws configure set aws_access_key_id $(echo $SECRET | jq -r '.AccessKeyId') --profile ota-uploader
aws configure set aws_secret_access_key $(echo $SECRET | jq -r '.SecretAccessKey') --profile ota-uploader

# For Windows (PowerShell):
$secret = aws secretsmanager get-secret-value --secret-id my-device-uploader-credentials-dev --query SecretString --output text | ConvertFrom-Json
aws configure set aws_access_key_id $secret.AccessKeyId --profile ota-uploader
aws configure set aws_secret_access_key $secret.SecretAccessKey --profile ota-uploader

# Upload to S3 (using the ota-uploader profile)
aws s3 cp firmware.bin s3://your-bucket/firmware/v1.1.0.bin --profile ota-uploader
aws s3 cp manifest.json s3://your-bucket/manifest.json --profile ota-uploader

# For CloudFront (public-prod), invalidate cache after upload
aws cloudfront create-invalidation --distribution-id YOUR_DIST_ID --paths "/*"
```

## Authentication Modes

### PUBLIC Access
```cpp
#define OTA_AUTH_MODE PUBLIC_ACCESS
```
- **Security**: None - anyone can download
- **Setup**: No additional configuration

### SHARED_KEY
```cpp
#define OTA_AUTH_MODE SHARED_KEY
#define OTA_SHARED_KEY "your-project-key-2024"
```
- **Security**: Shared secret in firmware
- **Limitation**: Cannot revoke keys once deployed

### DEVICE_SPECIFIC
```cpp
#define OTA_AUTH_MODE DEVICE_SPECIFIC
```
- **Security**: Unique credentials per device
- **Features**: Device provisioning, revocation, audit

## API Reference

### Web Interface
- **GET** `/ota/` - Main OTA management interface
- **GET** `/ota/assets/ota-utils.js` - JavaScript utilities

### REST API Endpoints

#### Status & Information
```http
GET /ota/api/status
GET /ota/api/manifest  
GET /ota/api/history
GET /ota/api/progress
```

#### Update Operations
```http
POST /ota/api/check      # Check for updates
POST /ota/api/install    # Install latest version
```

#### Development Mode Only
```http
POST /ota/api/rollback              # Rollback firmware
POST /ota/api/install/{version}     # Install specific version
```

### Example API Usage

```javascript
// Check for updates
const response = await fetch('/ota/api/check', { method: 'POST' });
const result = await response.json();

// Install update
const installResponse = await fetch('/ota/api/install', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ version: '2.1.0' })
});
```

## Configuration Options

### Build-Time Configuration
```cpp
// Authentication mode (required)
#define OTA_AUTH_MODE PUBLIC_ACCESS|SHARED_KEY|DEVICE_SPECIFIC

// Current firmware version of your application (required)
#define OTA_FIRMWARE_VERSION "1.2.3"

// Manifest URL (required)
#define OTA_MANIFEST_URL "https://your-cdn.cloudfront.net/manifest.json"

// Shared key (required for SHARED_KEY mode)
#define OTA_SHARED_KEY "your-project-key"

// Development features
#define OTA_DEVELOPMENT_MODE        // Allow downgrades and rollback
#define OTA_SEMANTIC_STRICT         // Stage major version updates
```

### Runtime Configuration
```cpp
JsonDocument config;
config["auto_check"] = true;                    // Enable automatic update checks

// Authentication credentials (for DEVICE_SPECIFIC mode only)
config["device_id"] = "device-12345";
config["device_key"] = "unique-device-secret";

// If your platform supports config injection at registration time
// prefer passing the config when calling registerModule() as shown above.

// Optional: Set auto-check interval at runtime (defaults to 3600 seconds)
otaUpdateModule.setAutoCheckInterval(7200);     // Check every 2 hours
```

## AWS Infrastructure

### CloudFormation Templates
The `cloudformation/` directory includes multiple deployment templates:

**Public Access Templates:**
- `public-dev.yml` - Development with direct S3 access
- `public-prod.yml` - Production with CloudFront CDN

**Shared Key Templates:**
- `shared-key-dev.yml` - Development with key in firmware
- `shared-key-prod.yml` - Production with Lambda validation

**Device-Specific Template:**
- `device-specific.yml` - Enterprise with device registry

**⚠️ Important:** All templates now use proper S3 ARN formatting (e.g., `arn:aws:s3:::bucket-name/*`) to ensure successful CloudFormation stack creation. This was updated to fix IAM policy validation errors.

Each template creates:
- **S3 Bucket**: Firmware storage with versioning
- **CloudFront CDN**: Global distribution network
- **Lambda Functions**: Device authentication (enterprise modes)
- **API Gateway**: Device authentication endpoint
- **DynamoDB Table**: Device registry (enterprise modes)
- **IAM Roles/Users**: Secure access controls

### Cost Estimate (US East-1)
- **S3 Storage**: ~$0.023/GB/month
- **CloudFront**: ~$0.085/GB transferred
- **Lambda**: ~$0.20 per 1M requests
- **DynamoDB**: Pay-per-request pricing

### Supported Regions
Template supports all AWS regions with CloudFormation, S3, CloudFront, Lambda, and DynamoDB availability.

## Version Management

### Semantic Versioning
The module follows semantic versioning (semver) principles:
- **MAJOR**: Breaking changes that require user intervention
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes, backward compatible

### Update Policies
```cpp
// Production mode (default)
- Only allows forward updates (1.0.0 -> 1.1.0 -> 2.0.0)
- Blocks downgrades for safety

// Development mode
#define OTA_DEVELOPMENT_MODE
- Allows any version installation
- Enables manual rollback functionality

// Semantic strict mode  
#define OTA_SEMANTIC_STRICT
- Stages major version updates (1.x -> 2.0 -> 2.x)
- Prevents automatic major version jumps
```

## Security Best Practices

### General Guidelines
1. **Always use HTTPS** for manifest and firmware URLs
2. **Implement firmware signing** for additional security
3. **Use SHA-256 verification** (automatically enforced)
4. **Store sensitive keys securely** (AWS Secrets Manager recommended)
5. **Monitor update attempts** and implement alerting

### Device-Specific Mode
1. **Generate unique credentials** during manufacturing
2. **Implement device registration** workflow
3. **Use device revocation** for compromised devices
4. **Enable audit logging** with AWS CloudTrail
5. **Rotate device keys** periodically

### Infrastructure Security
1. **Enable CloudTrail** for all API calls
2. **Use IAM least-privilege** principles  
3. **Enable S3 bucket versioning** and MFA delete
4. **Set up CloudWatch alarms** for unusual activity
5. **Implement cost budgets** and alerts

## Troubleshooting

### Common Issues

#### 403 Forbidden Errors
```bash
# Check bucket policy and authentication mode
aws s3api get-bucket-policy --bucket your-bucket-name

# Test manifest access
curl -v https://your-cdn.cloudfront.net/manifest.json
```

#### SSL/TLS Errors
```cpp
// ESP32 SSL issues
httpClient.setInsecure();  // For testing only!

// Proper certificate validation
httpClient.setCACert(root_ca);
```

#### Memory Issues
```cpp
// Monitor memory usage
DEBUG_PRINTF("Free heap: %d bytes\n", ESP.getFreeHeap());

// Use streaming for large downloads
Update.onProgress(progressCallback);
```

### Debug Logging
```cpp
// Enable detailed logging
#define CORE_DEBUG_LEVEL 5

// OTA-specific debug messages
otaUpdateModule.setProgressCallback([](int progress, int total, String status) {
    Serial.printf("OTA: %s (%d/%d)\n", status.c_str(), progress, total);
});
```

## Development Mode Features

Enable development mode for testing and debugging:

```cpp
#define OTA_DEVELOPMENT_MODE
```

### Additional Features
- **Version Downgrade**: Install older firmware versions
- **Manual Rollback**: `/ota/api/rollback` endpoint
- **Specific Version Install**: `/ota/api/install/{version}`
- **Enhanced Logging**: Detailed debug output
- **Skip Verification**: Optional integrity bypass (testing only)

### Safety Warnings
⚠️ **Never use development mode in production**
⚠️ **Development features bypass safety checks**
⚠️ **Can brick devices if misused**

#### CloudFormation Deployment Errors
```bash
# If stack creation fails, check CloudFormation events
aws cloudformation describe-stack-events \
  --stack-name my-device-ota-dev \
  --max-items 20

# Check bucket name conflicts
aws s3api list-buckets | grep my-device

# Delete failed stack and retry
aws cloudformation delete-stack --stack-name my-device-ota-dev
```

### Issue Reporting
Please include:
- ESP32 board type and firmware version
- Authentication mode and configuration
- Complete error messages and stack traces
- Steps to reproduce the issue
- CloudFormation template version (if infrastructure-related)

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Support

- **GitHub Issues**: Bug reports and feature requests
- **Documentation**: See `OTA_PLAN.md` for implementation details
- **Examples**: Check `examples/` directory for usage patterns
- **CloudFormation**: See `cloudformation/deployment-guide.md`

---

**Made with ❤️ for the ESP32 community**
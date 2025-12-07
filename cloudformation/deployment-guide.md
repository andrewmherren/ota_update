# OTA Infrastructure Deployment Guide

⚠️ **DEPRECATED**: This guide covers the old monolithic CloudFormation template.

**🎆 Please use the new focused templates instead:**
- See [README.md](README.md) for the new template architecture
- Each authentication mode now has its own dedicated template
- Simpler to deploy, understand, and maintain

---

## Overview (Legacy)
This guide walks through deploying AWS infrastructure for ESP32 OTA updates using the old monolithic CloudFormation template. **Please use the new focused templates for new deployments.**

## Prerequisites
- AWS CLI installed and configured
- AWS account with appropriate permissions
- Basic understanding of CloudFormation

## Quick Start

### 1. Deploy Infrastructure

#### Public Access Mode (Simplest)
```bash
aws cloudformation create-stack \
  --stack-name my-device-ota \
  --template-body file://ota-infrastructure.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-iot-device \
               ParameterKey=AuthenticationMode,ParameterValue=PUBLIC \
  --capabilities CAPABILITY_NAMED_IAM
```

#### Shared Key Mode (Basic Protection)
```bash
aws cloudformation create-stack \
  --stack-name my-device-ota \
  --template-body file://ota-infrastructure.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-iot-device \
               ParameterKey=AuthenticationMode,ParameterValue=SHARED_KEY \
               ParameterKey=SharedKey,ParameterValue=your-secret-key-2024 \
  --capabilities CAPABILITY_NAMED_IAM
```

#### Device-Specific Mode (Enterprise)
```bash
aws cloudformation create-stack \
  --stack-name my-device-ota \
  --template-body file://ota-infrastructure.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-iot-device \
               ParameterKey=AuthenticationMode,ParameterValue=DEVICE_SPECIFIC \
  --capabilities CAPABILITY_NAMED_IAM
```

### 2. Get Deployment Outputs
```bash
aws cloudformation describe-stacks \
  --stack-name my-device-ota \
  --query 'Stacks[0].Outputs'
```

Important outputs:
- **ManifestURL**: Use this in your ESP32 firmware configuration
- **UploaderAccessKeyId/UploaderSecretAccessKey**: Use for CI/CD firmware uploads

### 3. Upload Your First Firmware

#### Create manifest.json
```json
{
  "product": "my-iot-device",
  "versions": {
    "1.0.0": {
      "url": "https://your-cdn-domain.cloudfront.net/firmware/v1.0.0.bin",
      "sha256": "your-firmware-sha256-hash",
      "released": "2024-01-15T10:30:00Z",
      "size_bytes": 1048576,
      "notes": "Initial release"
    }
  },
  "latest": "1.0.0",
  "minimum_supported": "1.0.0"
}
```

#### Upload files to S3
```bash
# Upload firmware binary
aws s3 cp firmware.bin s3://your-bucket-name/firmware/v1.0.0.bin

# Upload manifest
aws s3 cp manifest.json s3://your-bucket-name/manifest.json \
  --content-type application/json
```

## Authentication Modes

### PUBLIC Access
- **Use Case**: Open source projects, hobbyist development
- **Security**: None - anyone can download firmware
- **Setup**: No additional configuration needed
- **Firmware Config**: 
  ```cpp
  #define OTA_AUTH_MODE PUBLIC_ACCESS
  ```

### SHARED_KEY Access
- **Use Case**: Small commercial projects with basic protection
- **Security**: Shared secret embedded in firmware
- **Limitations**: Key cannot be revoked once devices are deployed
- **Setup**: Provide `SharedKey` parameter during deployment
- **Firmware Config**:
  ```cpp
  #define OTA_AUTH_MODE SHARED_KEY
  #define OTA_SHARED_KEY "your-secret-key-2024"
  ```

### DEVICE_SPECIFIC Access
- **Use Case**: Enterprise deployments with device registration
- **Security**: Unique credentials per device
- **Features**: Device registration, revocation, audit trail
- **Setup**: Requires device provisioning workflow
- **Firmware Config**:
  ```cpp
  #define OTA_AUTH_MODE DEVICE_SPECIFIC
  // Device ID and key provisioned during manufacturing
  ```

### SIGNED_URLS Access (Future)
- **Use Case**: Commercial with time-limited access
- **Security**: Presigned URLs with expiration
- **Features**: Automatic URL expiration, rate limiting
- **Firmware Config**:
  ```cpp
  #define OTA_AUTH_MODE SIGNED_URLS
  ```

## Firmware Integration

### Basic Configuration
```cpp
// In your main.cpp or configuration file
#include <ota_update_module.h>

void setup() {
  // Initialize web platform
  webPlatform.begin("MyDevice");
  
  // Configure OTA module
  JsonDocument config;
  config["manifest_url"] = "https://your-cdn-domain.cloudfront.net/manifest.json";
  config["auto_check"] = false;  // Manual checks only
  config["check_interval"] = 3600;  // 1 hour if auto_check enabled
  
  otaUpdateModule.begin(config.as<JsonVariant>());
  
  // Register OTA module
  webPlatform.registerModule("/ota", &otaUpdateModule);
}

void loop() {
  webPlatform.handle();
}
```

### Build Flags
```ini
# platformio.ini
[env:esp32]
build_flags = 
  -DOTA_AUTH_MODE=PUBLIC_ACCESS
  -DOTA_FIRMWARE_VERSION="1.0.0"
  # Optional flags:
  # -DOTA_DEVELOPMENT_MODE  # Allow downgrades and rollback
  # -DOTA_SEMANTIC_STRICT   # Stage major version updates
```

## CI/CD Integration

### GitHub Actions Example
```yaml
name: Build and Deploy Firmware

on:
  push:
    tags:
      - 'v*'

jobs:
  build-and-deploy:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Build firmware
        run: |
          platformio run --environment esp32
          
      - name: Calculate SHA256
        id: hash
        run: |
          HASH=$(sha256sum .pio/build/esp32/firmware.bin | cut -d' ' -f1)
          echo "firmware_hash=$HASH" >> $GITHUB_OUTPUT
          
      - name: Upload firmware
        env:
          AWS_ACCESS_KEY_ID: ${{ secrets.AWS_ACCESS_KEY_ID }}
          AWS_SECRET_ACCESS_KEY: ${{ secrets.AWS_SECRET_ACCESS_KEY }}
        run: |
          VERSION=${GITHUB_REF#refs/tags/v}
          aws s3 cp .pio/build/esp32/firmware.bin \
            s3://your-bucket/firmware/v${VERSION}.bin
            
      - name: Update manifest
        env:
          AWS_ACCESS_KEY_ID: ${{ secrets.AWS_ACCESS_KEY_ID }}
          AWS_SECRET_ACCESS_KEY: ${{ secrets.AWS_SECRET_ACCESS_KEY }}
        run: |
          # Script to update manifest.json with new version
          ./scripts/update-manifest.sh ${VERSION} ${{ steps.hash.outputs.firmware_hash }}
```

## Cost Optimization

### Expected Costs (US East-1)
- **S3 Storage**: $0.023/GB/month for first 50TB
- **CloudFront**: $0.085/GB for first 10TB/month
- **Lambda**: $0.20 per 1M requests (device-specific mode only)
- **DynamoDB**: Pay-per-request pricing (device-specific mode only)

### Optimization Tips
1. Use CloudFront's cheapest price class for global distribution
2. Enable S3 lifecycle policies to delete old firmware versions
3. Use compression for firmware binaries
4. Consider regional deployments for very low traffic

## Security Best Practices

### General
1. Always use HTTPS for all communications
2. Implement firmware signing and verification
3. Use SHA-256 for firmware integrity verification
4. Store sensitive credentials securely (AWS Secrets Manager)

### Device-Specific Mode
1. Generate unique device credentials during manufacturing
2. Implement device registration workflow
3. Monitor authentication attempts
4. Implement device revocation mechanism
5. Use AWS CloudTrail for audit logging

### Infrastructure
1. Enable CloudTrail for all API calls
2. Use IAM least-privilege principle
3. Enable S3 bucket versioning and MFA delete
4. Monitor costs and usage with AWS Budgets
5. Set up CloudWatch alarms for unusual activity

## Troubleshooting

### Common Issues
1. **403 Forbidden**: Check bucket policy and authentication mode
2. **CORS Errors**: Ensure proper CORS configuration in S3 bucket
3. **SSL Errors**: Verify certificate configuration on ESP32
4. **Manifest Parse Errors**: Validate JSON format and encoding
5. **Upload Failures**: Check IAM permissions for uploader user

### Debug Commands
```bash
# Test manifest access
curl -v https://your-cdn-domain.cloudfront.net/manifest.json

# Test firmware download
curl -v https://your-cdn-domain.cloudfront.net/firmware/v1.0.0.bin

# Check S3 bucket contents
aws s3 ls s3://your-bucket-name --recursive

# Check CloudFormation stack status
aws cloudformation describe-stack-events --stack-name my-device-ota
```

## Cleanup

### Delete Stack
```bash
# Empty S3 bucket first (due to versioning)
aws s3 rm s3://your-bucket-name --recursive
aws s3api delete-bucket-versions --bucket your-bucket-name

# Delete CloudFormation stack
aws cloudformation delete-stack --stack-name my-device-ota
```

## Next Steps
1. Implement automated firmware signing
2. Set up monitoring and alerting
3. Create device provisioning workflow
4. Integrate with existing device management systems
5. Implement fleet management features
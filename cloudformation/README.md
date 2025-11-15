# OTA Update CloudFormation Templates

This directory contains focused CloudFormation templates for different OTA authentication modes and deployment scenarios. Each template is designed for a specific use case, making them easier to understand, deploy, and maintain.

## Template Overview

| Template | Use Case | Features | Cost |
|----------|----------|----------|------|
| `public-dev.yml` | Development/Testing | S3 bucket, public access | $0.023/GB |
| `public-prod.yml` | Production Open Source | S3 + CloudFront CDN | $0.085/GB + CDN |
| `shared-key-dev.yml` | Small Projects (Dev) | S3 public-read (key in firmware, not enforced by AWS) | $0.023/GB |
| `shared-key-prod.yml` | Small Projects (Prod) | S3 + Lambda + API Gateway | $0.40/1M requests |
| `device-specific.yml` | Enterprise | Full authentication stack | $1.25/1M requests |

## Quick Start

### 1. PUBLIC_ACCESS Mode (Simplest)

**For Development:**
```bash
aws cloudformation create-stack \
  --stack-name my-ota-public-dev \
  --template-body file://public-dev.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device
```

**For Production:**
```bash
aws cloudformation create-stack \
  --stack-name my-ota-public-prod \
  --template-body file://public-prod.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device
```

### 2. SHARED_KEY Mode

**For Development:**
```bash
aws cloudformation create-stack \
  --stack-name my-ota-shared-dev \
  --template-body file://shared-key-dev.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device \
               ParameterKey=SharedKey,ParameterValue=my-secret-key-2024
```

**For Production:**
```bash
aws cloudformation create-stack \
  --stack-name my-ota-shared-prod \
  --template-body file://shared-key-prod.yml \
  --parameters ParameterKey=ProductName,ParameterValue=my-device \
               ParameterKey=SharedKey,ParameterValue=my-secret-key-2024
```

### 3. DEVICE_SPECIFIC Mode (Enterprise)

```bash
aws cloudformation create-stack \
  --stack-name my-ota-device-prod \
  --template-body file://device-specific.yml \
  --capabilities CAPABILITY_NAMED_IAM \
  --parameters ParameterKey=ProductName,ParameterValue=my-device \
               ParameterKey=Environment,ParameterValue=prod
```

## Template Details

### public-dev.yml
- **Purpose**: Quick development setup with direct S3 access
- **Features**: Public S3 bucket with CORS enabled
- **Limitations**: No CDN, not suitable for production
- **Cost**: ~$0.023/GB storage + requests

### public-prod.yml  
- **Purpose**: Production deployment for open-source projects
- **Features**: S3 + CloudFront CDN for global distribution
- **Security**: Origin Access Control prevents direct S3 access
- **Cost**: ~$0.085/GB storage + CDN costs

### shared-key-dev.yml
- **Purpose**: Development with basic access control
- **Features**: S3 with conditional access based on object tags
- **Security**: Shared key validation, local network bypass for testing
- **Cost**: ~$0.023/GB storage + requests

### shared-key-prod.yml
- **Purpose**: Production deployment with shared key authentication
- **Features**: Lambda function validates keys and generates presigned URLs
- **Security**: Keys can't be revoked once deployed (limitation)
- **Cost**: ~$0.40/1M Lambda requests + S3 costs

### device-specific.yml
- **Purpose**: Enterprise deployment with per-device authentication
- **Features**: DynamoDB device registry, Lambda validation, admin API
- **Security**: Individual device keys, device deactivation, audit trail
- **Cost**: ~$1.25/1M Lambda requests + DynamoDB costs

## Configuration Examples

### Build Flags for Each Mode

**PUBLIC_ACCESS (Development):**
```ini
build_flags = 
  -DOTA_AUTH_MODE=PUBLIC_ACCESS
  -DOTA_FIRMWARE_VERSION="\"1.0.0\""
  ; Optional: enable dev features (downgrades, rollback endpoints)
  ; -DOTA_DEVELOPMENT_MODE=1
  -DOTA_MANIFEST_URL="https://my-bucket.s3.us-west-2.amazonaws.com/manifest.json"
```

**PUBLIC_ACCESS (Production):**
```ini
build_flags = 
  -DOTA_AUTH_MODE=PUBLIC_ACCESS
  -DOTA_FIRMWARE_VERSION="\"1.0.0\""
  -DOTA_MANIFEST_URL="https://d1234567890.cloudfront.net/manifest.json"
```

**SHARED_KEY (Development):**
```ini
build_flags = 
  -DOTA_AUTH_MODE=SHARED_KEY
  -DOTA_FIRMWARE_VERSION="\"1.0.0\""
  -DOTA_SHARED_KEY="my-secret-key-2024"
  ; Direct S3 (key not enforced by AWS in dev template)
  -DOTA_MANIFEST_URL="https://my-bucket.s3.us-west-2.amazonaws.com/manifest.json"
```

**SHARED_KEY (Production):**
```ini
build_flags = 
  -DOTA_AUTH_MODE=SHARED_KEY
  -DOTA_FIRMWARE_VERSION="\"1.0.0\""
  -DOTA_SHARED_KEY="my-secret-key-2024"
  -DOTA_MANIFEST_URL="https://api-id.execute-api.us-west-2.amazonaws.com/prod/manifest.json"
```

**DEVICE_SPECIFIC:**
```ini
build_flags = 
  -DOTA_AUTH_MODE=DEVICE_SPECIFIC
  -DOTA_FIRMWARE_VERSION="\"1.0.0\""
  ; Base must point to device auth endpoint with desired firmware path
  ; The OTA module will append device_id and device_key automatically
  -DOTA_MANIFEST_URL="https://api-id.execute-api.us-west-2.amazonaws.com/prod/auth?firmware_path=manifest.json"
```

## Deployment Workflow

### 1. Deploy Infrastructure
Choose and deploy the appropriate CloudFormation template for your use case.

### 2. Configure Build Flags
Update your `platformio.ini` with the output values from the CloudFormation stack.

### 3. Upload Firmware
Use the provided uploader credentials to upload your manifest and firmware files.

### 4. Test Update Flow
Deploy test firmware and verify the OTA update process works correctly.

## Cost Optimization

### Development
- Use `public-dev.yml` or `shared-key-dev.yml` for lowest cost
- No CDN or Lambda costs
- Pay only for S3 storage and requests

### Production - Low Volume
- Use `public-prod.yml` with CloudFront
- CDN provides global performance
- Cache reduces S3 requests

### Production - High Volume
- Use `device-specific.yml` for security
- DynamoDB scales automatically
- Lambda costs predictable per request

## Security Considerations

### PUBLIC_ACCESS
- ✅ Simple to deploy and manage
- ❌ No access control - anyone can download firmware
- ❌ Not suitable for proprietary firmware

### SHARED_KEY
- ✅ Basic access control
- ✅ Suitable for small to medium deployments
- ❌ Keys cannot be revoked once devices are deployed
- ❌ Single key compromise affects all devices

### DEVICE_SPECIFIC
- ✅ Individual device authentication
- ✅ Device deactivation capability
- ✅ Access audit trail
- ✅ Suitable for enterprise deployments
- ❌ More complex to deploy and manage
- ❌ Requires device provisioning workflow

## Troubleshooting

### Common Issues

**CloudFormation Stack Creation Failed:**
- Check IAM permissions for CloudFormation, S3, Lambda, and API Gateway
- Ensure the ProductName parameter is unique and follows naming conventions
- For DEVICE_SPECIFIC mode, include `--capabilities CAPABILITY_NAMED_IAM`

**Devices Can't Download Firmware:**
- Verify the manifest URL is correct and accessible
- Check S3 bucket policy allows the expected access pattern
- For SHARED_KEY (prod), ensure the SharedKey matches and API URL is correct
- For DEVICE_SPECIFIC mode, verify device registration and key validity

**High Costs:**
- Review CloudWatch logs for excessive Lambda invocations
- Check S3 access patterns - consider CloudFront for frequently accessed content
- Monitor DynamoDB usage in DEVICE_SPECIFIC mode

### Support Commands

**Get Stack Outputs:**
```bash
aws cloudformation describe-stacks --stack-name my-ota-stack --query 'Stacks[0].Outputs'
```

**Test Device Authentication (DEVICE_SPECIFIC):**
```bash
curl -X POST https://api-id.execute-api.region.amazonaws.com/prod/auth \
  -H "Content-Type: application/json" \
  -d '{"device_id":"ESP32-001","device_key":"device-key","firmware_path":"manifest.json"}'
```

**Register New Device (DEVICE_SPECIFIC):**
```bash
curl -X POST https://api-id.execute-api.region.amazonaws.com/prod/admin \
  -H "Content-Type: application/json" \
  -d '{"action":"register","device_id":"ESP32-001","device_type":"esp32"}'
```

## Migration Between Templates

### From Development to Production
1. Deploy production template
2. Copy firmware files to new bucket
3. Update build flags with new URLs
4. Test thoroughly before switching production devices

### From PUBLIC to SHARED_KEY
1. Deploy SHARED_KEY template
2. Copy firmware with appropriate tags
3. Update build flags to include shared key
4. Deploy new firmware to devices

### From SHARED_KEY to DEVICE_SPECIFIC
1. Deploy DEVICE_SPECIFIC template
2. Register all devices using admin API
3. Update firmware build flags
4. Deploy new firmware with device credentials
5. Decommission old infrastructure

## Best Practices

1. **Use Separate Stacks**: Deploy dev/staging/prod as separate CloudFormation stacks
2. **Version Control**: Store CloudFormation templates in version control
3. **Parameter Validation**: Use CloudFormation parameters for environment-specific values
4. **Monitoring**: Set up CloudWatch alarms for Lambda errors and DynamoDB throttling
5. **Backup**: Enable point-in-time recovery for DynamoDB in production
6. **Testing**: Always test updates in development environment first
7. **Documentation**: Document your specific parameter values and deployment procedures

## Next Steps

After deploying your chosen template:

1. **Set up CI/CD**: Automate firmware uploads using the provided credentials
2. **Monitor Usage**: Set up CloudWatch dashboards for your deployment
3. **Plan Scaling**: Consider when to migrate to more sophisticated authentication modes
4. **Security Review**: Regularly review access patterns and rotate credentials
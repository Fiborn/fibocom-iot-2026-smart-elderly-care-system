import base64
import threading
import traceback
import sys
from flask import Flask, request, jsonify
from flask_cors import CORS
from obs import ObsClient
from obs.model import CompleteMultipartUploadRequest, CompletePart, PutObjectHeader

app = Flask(__name__)
CORS(app)

# ================= 华为云 OBS 配置 =================
ACCESS_KEY_ID = 'HPUASYSNUNKDQLNQFFCA'
SECRET_ACCESS_KEY = 'ONVgJEyKRlicWGWl5D5uyxhjkITzuOxAoBXMn44Z'
ENDPOINT = 'https://obs.cn-north-4.myhuaweicloud.com'
BUCKET_NAME = 'l610pic'
# ===================================================

# OBS分段上传最小分片大小（5MB）
MIN_PART_SIZE = 5 * 1024 * 1024  # 5MB

obs_client = ObsClient(
    access_key_id=ACCESS_KEY_ID,
    secret_access_key=SECRET_ACCESS_KEY,
    server=ENDPOINT
)

# 分段上传会话
upload_sessions = {}
# 普通上传缓存（用于小文件）
small_file_cache = {}
session_lock = threading.Lock()

@app.route('/upload_chunk', methods=['POST'])
def upload_chunk():
    try:
        print("\n" + "="*60)
        print("📥 Received upload_chunk request")
        
        # 1. 解析请求
        data = request.get_json()
        if not data:
            print("✗ No JSON data received")
            return jsonify({"status": "fail", "message": "No data received"}), 400
        
        filename = data.get("filename")
        chunk_index = data.get("chunk_index")
        total_chunks = data.get("total_chunks")
        chunk_base64 = data.get("image_data")
        
        print(f"📄 Filename: {filename}")
        print(f"📦 Chunk: {chunk_index + 1}/{total_chunks}")
        
        if filename is None or chunk_index is None or total_chunks is None or not chunk_base64:
            print("✗ Missing required fields")
            return jsonify({"status": "fail", "message": "Missing required fields"}), 400

        chunk_index = int(chunk_index)
        total_chunks = int(total_chunks)

        # 2. 解码Base64
        if ',' in chunk_base64:
            chunk_base64 = chunk_base64.split(',', 1)[1]
        
        try:
            chunk_bytes = base64.b64decode(chunk_base64)
            chunk_size = len(chunk_bytes)
            print(f"✓ Decoded {chunk_size} bytes ({chunk_size/1024:.2f} KB)")
        except Exception as decode_err:
            print(f"✗ Base64 decode error: {decode_err}")
            return jsonify({"status": "fail", "message": f"Decode failed: {str(decode_err)}"}), 400

        # 3. 判断使用分段上传还是普通上传
        use_simple_upload = chunk_size < MIN_PART_SIZE
        
        if use_simple_upload:
            print(f"📌 Using simple upload mode (chunk < 5MB)")
            return handle_simple_upload(filename, chunk_index, total_chunks, chunk_bytes)
        else:
            print(f"📌 Using multipart upload mode (chunk >= 5MB)")
            return handle_multipart_upload(filename, chunk_index, total_chunks, chunk_bytes)

    except Exception as e:
        print(f"\n✗ CRITICAL ERROR:")
        traceback.print_exc()
        return jsonify({"status": "error", "message": str(e)}), 500


def handle_simple_upload(filename, chunk_index, total_chunks, chunk_bytes):
    """处理小文件的普通上传（在内存中拼接所有分片）"""
    try:
        with session_lock:
            if filename not in small_file_cache:
                small_file_cache[filename] = {
                    "total_chunks": total_chunks,
                    "chunks": {}
                }
            
            # 保存当前分片
            small_file_cache[filename]["chunks"][chunk_index] = chunk_bytes
            current_uploaded = len(small_file_cache[filename]["chunks"])
        
        print(f"📊 Cache Progress: {current_uploaded}/{total_chunks}")
        
        # 检查是否收集完所有分片
        if current_uploaded < total_chunks:
            return jsonify({
                "status": "processing",
                "message": f"Chunk {chunk_index + 1}/{total_chunks} received (simple mode)",
                "uploaded": current_uploaded,
                "total": total_chunks
            }), 200
        
        # 所有分片已收集完毕，开始拼接并上传
        print("🔗 All chunks received, assembling...")
        
        with session_lock:
            file_data = small_file_cache.pop(filename)
        
        # 按顺序拼接所有分片
        try:
            assembled_bytes = b"".join(
                file_data["chunks"][i] for i in range(total_chunks)
            )
            print(f"✓ Assembled {len(assembled_bytes)} bytes ({len(assembled_bytes)/1024:.2f} KB)")
        except KeyError as e:
            print(f"✗ Missing chunk in sequence: {e}")
            return jsonify({
                "status": "fail",
                "message": f"Missing chunk {e}"
            }), 400
        
        # 一次性上传到OBS
        print("⬆️  Uploading to OBS (simple mode)...")
        try:
            # 使用正确的方式传递 Content-Type
            headers = PutObjectHeader()
            headers.contentType = 'image/jpeg'
            
            resp = obs_client.putContent(
                bucketName=BUCKET_NAME,
                objectKey=filename,
                content=assembled_bytes,
                metadata=headers
            )
            
            if resp.status >= 300:
                print(f"✗ Upload failed: {resp.errorCode} - {resp.errorMessage}")
                return jsonify({
                    "status": "fail",
                    "message": f"Upload failed: {resp.errorMessage}"
                }), 500
            
            file_url = f"https://{BUCKET_NAME}.{ENDPOINT.replace('https://', '')}/{filename}"
            print(f"✅ SUCCESS! File uploaded to: {file_url}")
            print("="*60 + "\n")
            
            return jsonify({
                "status": "success",
                "message": "File uploaded successfully (simple mode)",
                "filename": filename,
                "url": file_url,
                "size": len(assembled_bytes)
            }), 200
            
        except Exception as upload_err:
            print(f"✗ Upload exception:")
            traceback.print_exc()
            return jsonify({
                "status": "fail",
                "message": f"Upload error: {str(upload_err)}"
            }), 500
    
    except Exception as e:
        print(f"✗ Simple upload error:")
        traceback.print_exc()
        return jsonify({"status": "error", "message": str(e)}), 500


def handle_multipart_upload(filename, chunk_index, total_chunks, chunk_bytes):
    """处理大文件的分段上传"""
    try:
        part_number = chunk_index + 1
        
        # 初始化分段上传
        with session_lock:
            if filename not in upload_sessions:
                print("🚀 Initiating multipart upload...")
                try:
                    resp = obs_client.initiateMultipartUpload(
                        bucketName=BUCKET_NAME,
                        objectKey=filename,
                        contentType='image/jpeg'
                    )
                    
                    if resp.status >= 300:
                        print(f"✗ Init failed: {resp.errorCode} - {resp.errorMessage}")
                        return jsonify({
                            "status": "fail",
                            "message": f"Init failed: {resp.errorMessage}"
                        }), 500
                    
                    upload_id = resp.body.uploadId
                    upload_sessions[filename] = {
                        "upload_id": upload_id,
                        "parts": {},
                        "total_chunks": total_chunks
                    }
                    print(f"✓ Upload ID: {upload_id}")
                    
                except Exception as init_err:
                    print(f"✗ Init exception:")
                    traceback.print_exc()
                    return jsonify({"status": "fail", "message": str(init_err)}), 500
            
            upload_id = upload_sessions[filename]["upload_id"]

        # 上传分片
        print(f"⬆️  Uploading part {part_number}...")
        try:
            resp = obs_client.uploadPart(
                bucketName=BUCKET_NAME,
                objectKey=filename,
                partNumber=part_number,
                uploadId=upload_id,
                content=chunk_bytes
            )
            
            if resp.status >= 300:
                print(f"✗ Upload failed: {resp.errorCode} - {resp.errorMessage}")
                return jsonify({
                    "status": "fail",
                    "message": f"Upload part failed: {resp.errorMessage}"
                }), 500
            
            etag = resp.body.etag
            print(f"✓ Part {part_number} uploaded, ETag: {etag}")
            
            with session_lock:
                upload_sessions[filename]["parts"][part_number] = etag
                current_uploaded = len(upload_sessions[filename]["parts"])
            
            print(f"📊 Progress: {current_uploaded}/{total_chunks}")
            
        except Exception as upload_err:
            print(f"✗ Upload exception:")
            traceback.print_exc()
            return jsonify({"status": "fail", "message": str(upload_err)}), 500

        # 检查是否所有分片都已上传
        if current_uploaded < total_chunks:
            print(f"⏳ Waiting for remaining {total_chunks - current_uploaded} parts...")
            return jsonify({
                "status": "processing",
                "message": f"Part {part_number}/{total_chunks} uploaded (multipart mode)",
                "uploaded": current_uploaded,
                "total": total_chunks
            }), 200

        # 完成分段上传
        print("🏁 All parts received, completing upload...")
        with session_lock:
            session_info = upload_sessions.pop(filename)
        
        try:
            parts_list = []
            print("\n📋 Building parts list:")
            for pn in range(1, total_chunks + 1):
                if pn not in session_info["parts"]:
                    print(f"✗ Missing part {pn}!")
                    with session_lock:
                        upload_sessions[filename] = session_info
                    return jsonify({
                        "status": "fail",
                        "message": f"Missing part {pn}"
                    }), 400
                
                etag = session_info["parts"][pn].strip('"')
                part = CompletePart(partNum=pn, etag=etag)
                parts_list.append(part)
                print(f"  Part {pn}: {etag}")
            
            print(f"\n🔗 Completing with {len(parts_list)} parts...")
            
            complete_request = CompleteMultipartUploadRequest(parts=parts_list)
            resp = obs_client.completeMultipartUpload(
                bucketName=BUCKET_NAME,
                objectKey=filename,
                uploadId=session_info["upload_id"],
                completeMultipartUploadRequest=complete_request
            )
            
            print(f"📡 Complete response: {resp.status} - {resp.reason}")
            
            if resp.status >= 300:
                print(f"✗ Complete failed: {resp.errorCode} - {resp.errorMessage}")
                
                try:
                    obs_client.abortMultipartUpload(
                        bucketName=BUCKET_NAME,
                        objectKey=filename,
                        uploadId=session_info["upload_id"]
                    )
                    print("🗑️  Upload aborted")
                except:
                    pass
                
                return jsonify({
                    "status": "fail",
                    "message": f"Complete failed: {resp.errorMessage}",
                    "error_code": resp.errorCode
                }), 500
            
            file_url = f"https://{BUCKET_NAME}.{ENDPOINT.replace('https://', '')}/{filename}"
            print(f"✅ SUCCESS! File uploaded to: {file_url}")
            print("="*60 + "\n")
            
            return jsonify({
                "status": "success",
                "message": "File uploaded successfully (multipart mode)",
                "filename": filename,
                "url": file_url
            }), 200
            
        except Exception as complete_err:
            print(f"\n✗ Complete exception:")
            traceback.print_exc()
            
            try:
                obs_client.abortMultipartUpload(
                    bucketName=BUCKET_NAME,
                    objectKey=filename,
                    uploadId=session_info["upload_id"]
                )
                print("🗑️  Upload aborted due to error")
            except:
                pass
            
            return jsonify({
                "status": "fail",
                "message": f"Complete error: {str(complete_err)}"
            }), 500
    
    except Exception as e:
        print(f"✗ Multipart upload error:")
        traceback.print_exc()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/cancel_upload', methods=['POST'])
def cancel_upload():
    """手动取消上传"""
    try:
        data = request.get_json()
        filename = data.get("filename")
        
        with session_lock:
            if filename in upload_sessions:
                session_info = upload_sessions.pop(filename)
                obs_client.abortMultipartUpload(
                    bucketName=BUCKET_NAME,
                    objectKey=filename,
                    uploadId=session_info["upload_id"]
                )
                return jsonify({
                    "status": "success",
                    "message": f"Multipart upload cancelled for {filename}"
                }), 200
            
            if filename in small_file_cache:
                small_file_cache.pop(filename)
                return jsonify({
                    "status": "success",
                    "message": f"Simple upload cache cleared for {filename}"
                }), 200
            
            return jsonify({
                "status": "fail",
                "message": "No active upload found"
            }), 404
    except Exception as e:
        traceback.print_exc()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/list_sessions', methods=['GET'])
def list_sessions():
    """查看当前上传会话"""
    with session_lock:
        result = {
            "multipart_uploads": {},
            "simple_uploads": {}
        }
        
        for filename, info in upload_sessions.items():
            result["multipart_uploads"][filename] = {
                "upload_id": info["upload_id"],
                "uploaded_parts": len(info["parts"]),
                "total_parts": info["total_chunks"],
                "parts": list(info["parts"].keys())
            }
        
        for filename, info in small_file_cache.items():
            result["simple_uploads"][filename] = {
                "uploaded_chunks": len(info["chunks"]),
                "total_chunks": info["total_chunks"],
                "chunks": list(info["chunks"].keys())
            }
    
    return jsonify(result), 200


@app.route('/test_obs', methods=['GET'])
def test_obs_connection():
    """测试OBS连接"""
    try:
        print("\n🧪 Testing OBS connection...")
        resp = obs_client.listBuckets()
        
        if resp.status < 300:
            buckets = [bucket.name for bucket in resp.body.buckets]
            print(f"✓ Buckets: {buckets}")
            
            return jsonify({
                "status": "success",
                "buckets": buckets,
                "target_bucket": BUCKET_NAME,
                "bucket_exists": BUCKET_NAME in buckets
            }), 200
        else:
            return jsonify({
                "status": "fail",
                "message": resp.errorMessage
            }), 500
    except Exception as e:
        traceback.print_exc()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/list_images', methods=['GET'])
def list_images():
    """列出OBS桶中的所有图片文件，返回签名URL"""
    try:
        print("\n📷 Listing images from OBS bucket...")
        resp = obs_client.listObjects(bucketName=BUCKET_NAME, max_keys=100)

        if resp.status >= 300:
            return jsonify({
                "status": "fail",
                "message": f"List failed: {resp.errorMessage}"
            }), 500

        image_extensions = ('.jpg', '.jpeg', '.png', '.gif', '.bmp', '.webp')
        images = []

        for obj in resp.body.contents:
            key = obj.key
            if key.lower().endswith(image_extensions):
                sign_resp = obs_client.createSignedUrl(
                    method='GET',
                    bucketName=BUCKET_NAME,
                    objectKey=key,
                    expires=3600
                )
                images.append({
                    "key": key,
                    "size": obj.size,
                    "lastModified": str(obj.lastModified),
                    "url": sign_resp.signedUrl
                })

        print(f"✓ Found {len(images)} images")
        return jsonify({
            "status": "success",
            "images": images
        }), 200

    except Exception as e:
        traceback.print_exc()
        return jsonify({"status": "error", "message": str(e)}), 500


@app.route('/health', methods=['GET'])
def health_check():
    """健康检查"""
    with session_lock:
        multipart_sessions = {k: len(v["parts"]) for k, v in upload_sessions.items()}
        simple_sessions = {k: len(v["chunks"]) for k, v in small_file_cache.items()}
    
    return jsonify({
        "status": "ok",
        "multipart_uploads": len(upload_sessions),
        "simple_uploads": len(small_file_cache),
        "multipart_sessions": multipart_sessions,
        "simple_sessions": simple_sessions
    }), 200


if __name__ == '__main__':
    print("=" * 60)
    print("🚀 OBS Upload Server (Hybrid Mode)")
    print(f"📡 Endpoint: {ENDPOINT}")
    print(f"🪣 Bucket: {BUCKET_NAME}")
    print(f"📏 Multipart threshold: {MIN_PART_SIZE/1024/1024:.0f}MB")
    print("=" * 60)
    print("\n📋 Features:")
    print("   ✓ Simple upload for small files (< 5MB per chunk)")
    print("   ✓ Multipart upload for large files (>= 5MB per chunk)")
    print("\n📋 Endpoints:")
    print("   POST /upload_chunk")
    print("   POST /cancel_upload")
    print("   GET  /list_sessions")
    print("   GET  /list_images")
    print("   GET  /test_obs")
    print("   GET  /health")
    print("\n" + "=" * 60 + "\n")
    
    app.run(host='0.0.0.0', port=5000, debug=True)
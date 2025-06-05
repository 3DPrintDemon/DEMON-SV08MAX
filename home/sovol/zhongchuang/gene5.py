import sys
from PIL import Image

def convert_png_to_jpg(in_path, out_path, size=64):
    try:
        with Image.open(in_path) as img:
            img = img.resize((size, size))
            img = img.rotate(180)
            img = img.convert("RGB")
            img.save(jpg_path, "JPEG", quality=60)
        print(f"成功将 {in_path} 转换为 {jpg_path}，并调整尺寸为 {size}x{size}")
    except Exception as e:
        print(f"转换失败: {e}")

# 从命令行获取参数
if len(sys.argv) < 4:
    print("请提供图片路径和输出路径作为命令行参数")
else:
    png_path = sys.argv[1]
    jpg_path = sys.argv[2]
    try:
        size = int(sys.argv[3])
    except ValueError:
        size = 64

    convert_png_to_jpg(png_path, jpg_path, size)


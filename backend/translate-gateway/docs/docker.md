# 启动服务器
docker run -d --name nats-server -p 4222:4222 nats:latest

# 拉取redis
docker pull redis

# 运行redis 容器
docker run --name my-redis -p 6379:6379 -e REDIS_PASSWORD=123456 -v redis-data:/data -d redis redis-server --requirepass 123456

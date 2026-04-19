-- Script to initialize 100,000 users
-- Usage: docker exec -i devops-db-1 psql -U postgres -d chess_db < 100k_user.sql

INSERT INTO users (username, password, role)
SELECT 
    'user_' || i, 
    'pass_' || i, 
    'user'
FROM generate_series(1, 100000) AS i
ON CONFLICT (username) DO NOTHING;

ANALYZE users;
SELECT 'Seeding 100,000 users completed.' as result;

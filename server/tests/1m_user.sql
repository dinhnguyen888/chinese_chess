-- Script to initialize 1,000,000 users
-- Usage: docker exec -i devops-db-1 psql -U postgres -d chess_db < 1m_user.sql

INSERT INTO users (username, password, role)
SELECT 
    'user_' || i, 
    'pass_' || i, 
    'user'
FROM generate_series(1, 1000000) AS i
ON CONFLICT (username) DO NOTHING;

ANALYZE users;
SELECT 'Seeding 1,000,000 users completed.' as result;

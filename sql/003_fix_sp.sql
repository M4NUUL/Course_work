CREATE TABLE IF NOT EXISTS assignment_students (
  id SERIAL PRIMARY KEY,
  assignment_id INTEGER NOT NULL REFERENCES assignments(id) ON DELETE CASCADE,
  student_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  UNIQUE (assignment_id, student_id)
);

CREATE OR REPLACE FUNCTION sp_get_my_submissions(p_student_id integer)
RETURNS TABLE(
  id integer,
  assignment_id integer,
  assignment_title text,
  original_name text,
  uploaded_at timestamp,
  grade text,
  feedback text,
  file_path text
)
LANGUAGE sql
AS $$
  SELECT
    s.id,
    s.assignment_id,
    a.title AS assignment_title,
    s.original_name,
    s.uploaded_at,
    s.grade,
    s.feedback,
    s.file_path
  FROM submissions s
  JOIN assignments a ON a.id = s.assignment_id
  WHERE s.student_id = p_student_id
  ORDER BY s.uploaded_at DESC;
$$;

CREATE OR REPLACE FUNCTION sp_delete_user(p_user_id integer, p_admin_id integer)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  DELETE FROM users WHERE id = p_user_id;

  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_admin_id, 'delete_user', concat('deleted_user=', p_user_id), now());
END;
$$;

CREATE OR REPLACE FUNCTION sp_delete_assignment(p_assignment_id integer, p_teacher_id integer)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  IF NOT EXISTS (
    SELECT 1
    FROM assignments
    WHERE id = p_assignment_id
      AND created_by = p_teacher_id
  ) THEN
    RAISE EXCEPTION 'forbidden';
  END IF;

  DELETE FROM assignments WHERE id = p_assignment_id;

  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_teacher_id, 'delete_assignment', concat('assignment_id=', p_assignment_id), now());
END;
$$;

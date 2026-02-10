CREATE OR REPLACE FUNCTION sp_get_assignments_for_student(p_student_id integer)
RETURNS TABLE(id integer, title text, due_date timestamp)
LANGUAGE sql
AS $$
  SELECT a.id, a.title, a.due_date
  FROM assignments a
  WHERE
    EXISTS (
      SELECT 1
      FROM assignment_students s
      WHERE s.assignment_id = a.id
        AND s.student_id = p_student_id
    )
    OR NOT EXISTS (
      SELECT 1
      FROM assignment_students s
      WHERE s.assignment_id = a.id
    )
  ORDER BY a.due_date NULLS LAST, a.id;
$$;

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

CREATE OR REPLACE FUNCTION sp_register_user(
  p_login text,
  p_role text,
  p_password_hash text,
  p_salt text
)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  INSERT INTO users (login, role, password_hash, salt, created_at, active)
  VALUES (p_login, p_role, p_password_hash, p_salt, CURRENT_TIMESTAMP, true);
END;
$$;

CREATE OR REPLACE FUNCTION sp_get_user_auth_data(p_login text)
RETURNS TABLE(user_id integer, password_hash text, salt text, role text)
LANGUAGE sql
AS $$
  SELECT id, password_hash, salt, role
  FROM users
  WHERE login = p_login AND active = true;
$$;

CREATE OR REPLACE FUNCTION sp_log_action(p_user_id integer, p_action text, p_details text)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_user_id, p_action, p_details, CURRENT_TIMESTAMP);
END;
$$;

CREATE OR REPLACE FUNCTION sp_create_submission(
  p_assignment_id integer,
  p_student_id integer,
  p_file_path text,
  p_original_name text
)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  INSERT INTO submissions (assignment_id, student_id, file_path, original_name, uploaded_at)
  VALUES (p_assignment_id, p_student_id, p_file_path, p_original_name, NOW());
END;
$$;

CREATE OR REPLACE FUNCTION sp_admin_list_users(p_admin_id integer)
RETURNS TABLE(user_id integer, login text, full_name text, role text)
LANGUAGE plpgsql
AS $$
BEGIN
  RETURN QUERY
    SELECT u.id, u.login, COALESCE(u.full_name, ''), u.role
    FROM users u
    ORDER BY u.id;
END;
$$;

CREATE OR REPLACE FUNCTION sp_admin_create_user(
  p_admin_id integer,
  p_login text,
  p_role text,
  p_full_name text,
  p_password_hash text,
  p_salt text
)
RETURNS integer
LANGUAGE plpgsql
AS $$
DECLARE
  v_user_id integer;
BEGIN
  INSERT INTO users (login, role, full_name, password_hash, salt, created_at, active)
  VALUES (p_login, p_role, NULLIF(p_full_name, ''), p_password_hash, p_salt, CURRENT_TIMESTAMP, true)
  RETURNING id INTO v_user_id;

  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_admin_id, 'create_user', concat('created_user=', v_user_id, ' login=', p_login, ' role=', p_role), now());

  RETURN v_user_id;
END;
$$;

CREATE OR REPLACE FUNCTION sp_admin_toggle_user_active(p_admin_id integer, p_user_id integer)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  UPDATE users SET active = NOT active WHERE id = p_user_id;

  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_admin_id, 'toggle_active', concat('user_id=', p_user_id), now());
END;
$$;

CREATE OR REPLACE FUNCTION sp_admin_get_user(p_admin_id integer, p_user_id integer)
RETURNS TABLE(login text, full_name text, role text)
LANGUAGE plpgsql
AS $$
BEGIN
  RETURN QUERY
    SELECT u.login, COALESCE(u.full_name, ''), u.role
    FROM users u
    WHERE u.id = p_user_id;
END;
$$;

CREATE OR REPLACE FUNCTION sp_admin_update_user(
  p_admin_id integer,
  p_user_id integer,
  p_full_name text,
  p_role text
)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  UPDATE users
  SET full_name = NULLIF(p_full_name, ''),
      role = p_role
  WHERE id = p_user_id;

  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_admin_id, 'edit_user', concat('user_id=', p_user_id), now());
END;
$$;

CREATE OR REPLACE FUNCTION sp_get_assignments_for_teacher(p_teacher_id integer)
RETURNS TABLE(id integer, title text)
LANGUAGE sql
AS $$
  SELECT a.id, a.title
  FROM assignments a
  WHERE a.created_by = p_teacher_id
  ORDER BY a.id DESC;
$$;

CREATE OR REPLACE FUNCTION sp_get_submissions_for_assignment(p_teacher_id integer, p_assignment_id integer)
RETURNS TABLE(
  id integer,
  student_login text,
  original_name text,
  uploaded_at timestamp,
  grade text,
  feedback text,
  file_path text
)
LANGUAGE plpgsql
AS $$
BEGIN
  IF NOT EXISTS (
    SELECT 1 FROM assignments
    WHERE id = p_assignment_id AND created_by = p_teacher_id
  ) THEN
    RAISE EXCEPTION 'forbidden';
  END IF;

  RETURN QUERY
    SELECT
      s.id,
      COALESCE(u.login, ''),
      s.original_name,
      s.uploaded_at,
      s.grade,
      s.feedback,
      s.file_path
    FROM submissions s
    LEFT JOIN users u ON s.student_id = u.id
    WHERE s.assignment_id = p_assignment_id
    ORDER BY s.uploaded_at DESC, s.id DESC;
END;
$$;

CREATE OR REPLACE FUNCTION sp_set_submission_grade(
  p_teacher_id integer,
  p_submission_id integer,
  p_grade text,
  p_feedback text
)
RETURNS void
LANGUAGE plpgsql
AS $$
DECLARE
  v_assignment_id integer;
BEGIN
  SELECT s.assignment_id INTO v_assignment_id
  FROM submissions s
  WHERE s.id = p_submission_id;

  IF v_assignment_id IS NULL THEN
    RAISE EXCEPTION 'not_found';
  END IF;

  IF NOT EXISTS (
    SELECT 1 FROM assignments
    WHERE id = v_assignment_id AND created_by = p_teacher_id
  ) THEN
    RAISE EXCEPTION 'forbidden';
  END IF;

  UPDATE submissions
  SET grade = NULLIF(p_grade, ''),
      feedback = NULLIF(p_feedback, '')
  WHERE id = p_submission_id;

  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_teacher_id, 'grade_submission', concat('submission_id=', p_submission_id), now());
END;
$$;

CREATE OR REPLACE FUNCTION sp_create_assignment(
  p_teacher_id integer,
  p_title text,
  p_description text,
  p_due_date timestamp
)
RETURNS integer
LANGUAGE plpgsql
AS $$
DECLARE
  v_id integer;
BEGIN
  INSERT INTO assignments (title, description, discipline_id, created_by, due_date, created_at)
  VALUES (p_title, NULLIF(p_description, ''), NULL, p_teacher_id, p_due_date, NOW())
  RETURNING id INTO v_id;

  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_teacher_id, 'create_assignment', concat('assignment_id=', v_id), now());

  RETURN v_id;
END;
$$;

CREATE OR REPLACE FUNCTION sp_list_students(p_teacher_id integer)
RETURNS TABLE(id integer, login text, full_name text)
LANGUAGE sql
AS $$
  SELECT u.id, u.login, COALESCE(u.full_name, '')
  FROM users u
  WHERE u.role = 'student'
  ORDER BY u.id;
$$;

CREATE OR REPLACE FUNCTION sp_assign_student_to_assignment(
  p_teacher_id integer,
  p_assignment_id integer,
  p_student_id integer
)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  IF NOT EXISTS (
    SELECT 1 FROM assignments
    WHERE id = p_assignment_id AND created_by = p_teacher_id
  ) THEN
    RAISE EXCEPTION 'forbidden';
  END IF;

  INSERT INTO assignment_students (assignment_id, student_id)
  VALUES (p_assignment_id, p_student_id)
  ON CONFLICT DO NOTHING;
END;
$$;

CREATE OR REPLACE FUNCTION sp_add_assignment_file(
  p_teacher_id integer,
  p_assignment_id integer,
  p_file_path text,
  p_original_name text
)
RETURNS void
LANGUAGE plpgsql
AS $$
BEGIN
  IF NOT EXISTS (
    SELECT 1 FROM assignments
    WHERE id = p_assignment_id AND created_by = p_teacher_id
  ) THEN
    RAISE EXCEPTION 'forbidden';
  END IF;

  INSERT INTO assignment_files (assignment_id, file_path, original_name, uploaded_at)
  VALUES (p_assignment_id, p_file_path, p_original_name, NOW());

  INSERT INTO audit_log(user_id, action, details, ts)
  VALUES (p_teacher_id, 'add_assignment_file', concat('assignment_id=', p_assignment_id), now());
END;
$$;

CREATE OR REPLACE FUNCTION sp_get_assignment_details(p_assignment_id integer)
RETURNS TABLE(title text, description text, due_date timestamp)
LANGUAGE sql
AS $$
  SELECT a.title, COALESCE(a.description, ''), a.due_date
  FROM assignments a
  WHERE a.id = p_assignment_id;
$$;

CREATE OR REPLACE FUNCTION sp_get_assignment_files(p_assignment_id integer)
RETURNS TABLE(id integer, original_name text, file_path text)
LANGUAGE sql
AS $$
  SELECT f.id, f.original_name, f.file_path
  FROM assignment_files f
  WHERE f.assignment_id = p_assignment_id
  ORDER BY f.uploaded_at DESC, f.id DESC;
$$;

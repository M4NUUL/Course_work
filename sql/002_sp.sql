-- 1) Ensure assignment_students exists
CREATE TABLE IF NOT EXISTS assignment_students (
  id SERIAL PRIMARY KEY,
  assignment_id INTEGER NOT NULL REFERENCES assignments(id) ON DELETE CASCADE,
  student_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  UNIQUE (assignment_id, student_id)
);

-- 2) Get assignments for student
CREATE OR REPLACE FUNCTION sp_get_assignments_for_student(p_student_id integer)
RETURNS TABLE(id integer, title text, due_date timestamp) LANGUAGE sql AS $$
    SELECT a.id, a.title, a.due_date
    FROM assignments a
    WHERE
      EXISTS (SELECT 1 FROM assignment_students s WHERE s.assignment_id = a.id AND s.student_id = p_student_id)
      OR NOT EXISTS (SELECT 1 FROM assignment_students s WHERE s.assignment_id = a.id)
    ORDER BY a.due_date;
$$;

-- 3) Delete assignment (teacher only)
CREATE OR REPLACE FUNCTION sp_delete_assignment(p_assignment_id integer, p_teacher_id integer)
RETURNS void LANGUAGE plpgsql AS $$
BEGIN
    IF NOT EXISTS (SELECT 1 FROM assignments WHERE id = p_assignment_id AND created_by = p_teacher_id) THEN
        RAISE EXCEPTION 'forbidden';
    END IF;
    DELETE FROM assignments WHERE id = p_assignment_id;
    INSERT INTO audit_log(user_id, action, details, ts)
      VALUES (p_teacher_id, 'delete_assignment', concat('assignment_id=', p_assignment_id), now());
END;
$$;

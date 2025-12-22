-- Создаём таблицу групп и таблицы для привязки
CREATE TABLE IF NOT EXISTS groups (
  id SERIAL PRIMARY KEY,
  name TEXT NOT NULL UNIQUE
);

DO $$
BEGIN
  IF NOT EXISTS (
    SELECT 1 FROM information_schema.columns
    WHERE table_name='users' and column_name='group_id'
  ) THEN
    ALTER TABLE users ADD COLUMN group_id INTEGER REFERENCES groups(id) ON DELETE SET NULL;
  END IF;
END$$;

CREATE TABLE IF NOT EXISTS assignment_groups (
  id SERIAL PRIMARY KEY,
  assignment_id INTEGER NOT NULL REFERENCES assignments(id) ON DELETE CASCADE,
  group_id INTEGER NOT NULL REFERENCES groups(id) ON DELETE CASCADE,
  UNIQUE (assignment_id, group_id)
);

CREATE TABLE IF NOT EXISTS assignment_files (
  id SERIAL PRIMARY KEY,
  assignment_id INTEGER REFERENCES assignments(id) ON DELETE CASCADE,
  file_path TEXT NOT NULL,
  original_name TEXT NOT NULL,
  uploaded_by INTEGER,
  uploaded_at TIMESTAMP DEFAULT now()
);

-- Обновим sp_get_assignments_for_student, чтобы учитывать назначения по группе
CREATE OR REPLACE FUNCTION sp_get_assignments_for_student(p_student_id integer)
RETURNS TABLE(id integer, title text, due_date timestamp) LANGUAGE sql AS $$
  SELECT DISTINCT a.id, a.title, a.due_date
  FROM assignments a
  LEFT JOIN assignment_students s ON s.assignment_id = a.id
  LEFT JOIN assignment_groups ag ON ag.assignment_id = a.id
  WHERE
    EXISTS (SELECT 1 FROM assignment_students s2 WHERE s2.assignment_id = a.id AND s2.student_id = p_student_id)
    OR (ag.group_id IS NOT NULL AND ag.group_id = (SELECT group_id FROM users WHERE id = p_student_id))
    OR (
       NOT EXISTS (SELECT 1 FROM assignment_students s3 WHERE s3.assignment_id = a.id)
       AND NOT EXISTS (SELECT 1 FROM assignment_groups ag2 WHERE ag2.assignment_id = a.id)
    )
  ORDER BY a.due_date;
$$;

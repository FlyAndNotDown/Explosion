import { Route, Routes } from 'react-router-dom';
import ProjectHubPage from '@/pages/project-hub';
import PrototypePage from '@/pages/prototype';
import OutlinerPage from '@/pages/editor/outliner';
import InspectorPage from '@/pages/editor/inspector';
import LogPage from '@/pages/editor/log';

function App() {
  return (
    <Routes>
      <Route element={<ProjectHubPage />} path='/project-hub' />
      <Route element={<PrototypePage />} path='/prototype' />
      <Route element={<OutlinerPage />} path='/editor/outliner' />
      <Route element={<InspectorPage />} path='/editor/inspector' />
      <Route element={<LogPage />} path='/editor/log' />
    </Routes>
  );
}

export default App;
